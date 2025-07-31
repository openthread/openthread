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

#ifndef OT_CORE_THREAD_EXT_NETWORK_DIAGNOSTIC_HPP_
#define OT_CORE_THREAD_EXT_NETWORK_DIAGNOSTIC_HPP_

#include <openthread/ext_network_diagnostic.h>

#include "common/array.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/owned_ptr.hpp"
#include "config/ext_network_diagnostic.h"
#include "net/ip6_address.hpp"
#include "thread/ext_network_diagnostic_types.hpp"
#include "thread/tmf.hpp"
#include "thread/uri_paths.hpp"

namespace ot {

#if OPENTHREAD_FTD
class Child;
class Router;
#endif

namespace ExtNetworkDiagnostic {

class Client;

/**
 * Implements the Extended Network Diagnostic server functionality for both routers and end devices.
 */
class Server : public InstanceLocator, private NonCopyable
{
private:
    friend class ot::TimeTicker;
    friend class Tmf::Agent;

    static constexpr uint16_t kCacheBuffersLimit    = OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CACHE_BUFFERS_LIMIT;
    static constexpr uint32_t kRegistrationInterval = OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_REGISTRATION_INTERVAL;
    static constexpr uint32_t kUpdateBaseDelay      = OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_UPDATE_BASE_DELAY;
    static constexpr uint32_t kUpdateExtDelay       = OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_UPDATE_EXT_DELAY;
    static constexpr uint32_t kUpdateJitter         = OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_UPDATE_JITTER;

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
    static constexpr otExtNetworkDiagnosticTlvSet kExtDelayTlvMask{{
        {(1U << Tlv::kLastHeard) | (1U << Tlv::kConnectionTime) | (1U << Tlv::kLinkMarginIn),
         (1U << (Tlv::kMacLinkErrorRatesOut - 8U)), 0,
         (1U << (Tlv::kMacCounters - 24U)) | (1U << (Tlv::kMacLinkErrorRatesIn - 24U)) |
             (1U << (Tlv::kMleCounters - 24U)) | (1U << (Tlv::kLinkMarginOut - 24U))},
    }};

    /**
     * Bitmask of TLVs which are static in neighbors and therefore do not need
     * to be sent in updates.
     */
    static constexpr otExtNetworkDiagnosticTlvSet kStaticNeighborTlvMask{{
        {(1U << Tlv::kMacAddress) | (1U << Tlv::kConnectionTime), 0U, (1U << (Tlv::kThreadSpecVersion - 16U)), 0U},
    }};

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
        /**
         * Represents the state of the diagnostic server on a child.
         */
        enum DiagState : uint8_t
        {
            kDiagServerStopped = 0,   ///< The diagnostic server is stopped
            kDiagServerActive,        ///< The diagnostic server is active
            kDiagServerStopPending,   ///< A stop command to the child is pending a response
            kDiagServerActivePending, ///< An active command to the child is pending a response
            kDiagServerUnknown,       ///< The last command to the child was not acked
        };

        /**
         * Marks the specified host provided TLV as having changed state.
         *
         * TLVs not provided by the host will be filtered out.
         */
        void MarkHostProvidedTlvsChanged(TlvSet aTlvs);

        /**
         * Marks the child as a full thread device.
         *
         * Note: This is duplicated from the child state itself since this class
         * has no access to the child class it belongs to. However since it only
         * requires a single bit and significantly improves the ability for
         * encapsulation it is done here anyways.
         */
        void SetChildIsFtd(bool aFtd) { mIsFtd = aFtd; }

        DiagState GetDiagServerState(void) const { return static_cast<DiagState>(mState); }

        /**
         * Indicates whether a extended network diagnostic command is pending.
         *
         * @returns TRUE if a command is pending, FALSE otherwise.
         */
        bool IsDiagServerPending(void) const
        {
            return (mState == kDiagServerActivePending) || (mState == kDiagServerStopPending);
        }

        void SetDiagServerState(DiagState aState) { mState = aState; }

        bool IsAttachStateDirty(void) const { return mAttachStateDirty; }
        void SetAttachStateDirty(void) { mAttachStateDirty = true; }

        uint16_t GetUsedCacheBuffers(void) const { return mCacheBuffers; }

        /**
         * Prepares the diagnostic cache to send an update to clients.
         *
         * MUST be match with a later call to either `CommitCacheUpdate`
         * or `AbortCacheUpdate`.
         */
        void LockCache(void)
        {
            OT_ASSERT(!mDiagCacheLocked);
            mDiagCacheLocked = true;
        }
        void CommitCacheUpdate(void);
        void AbortCacheUpdate(void);

        bool IsDiagCacheLocked(void) const { return mDiagCacheLocked; }

        /**
         * Indicates whether a diagnostic update should be sent to the server.
         *
         * @returns TRUE if a diagnostic update should be sent, FALSE otherwise.
         */
        bool ShouldSendDiagUpdate(void) const { return !mDirtySet.IsEmpty() || mAttachStateDirty; }

        TlvSet GetDirtyHostProvided(TlvSet aFilter) const;

        bool CanEvictCache(void) const { return (mCache != nullptr) && !mDiagCacheLocked; }

        void EvictCache(void);
        void ClearCache(void);

        /**
         * Updates the diag cache with an update message from the child.
         */
        Error UpdateCache(const Message &aMessage, TlvSet aFilter);

        /**
         * Appends the current diag cache to the message.
         *
         * MUST be called within a diagnostic update block by first calling
         * `LockCache`.
         */
        Error AppendCachedTlvs(Message &aMessage);

        void SetDiagClean(void);

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
    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * Starts the diagnostic server as a child device and configures enabled TLV types.
     *
     * This method initializes the diagnostic server if not already active and configures which
     * Type-Length-Value (TLV) diagnostic information types should be enabled for reporting.
     * It filters the requested TLVs to only include those valid for child devices and marks
     * appropriate TLVs as dirty to trigger updates.
     *
     * @param[in] aTypes  A set of TLV types to enable for diagnostic reporting.
     *
     * @retval true   The enabled TLV set has changed from the previous configuration.
     * @retval false  The enabled TLV set remains unchanged.
     *
     * @note When starting for the first time (mActive == false), initializes all tracking state
     *       including enabled, pending update, and dirty sets. For FTD builds, also clears
     *       child and neighbor enabled sets.
     *
     * @note The method filters incoming TLVs to child-valid types and schedules an update timer
     *       with random jitter if any TLVs are marked dirty.
     *
     * @note In FTD builds, child-provided TLVs (either MTD or FTD specific) are automatically
     *       marked dirty to ensure they are sent via EU (Enhanced Update) when requested.
     */
    bool ConfigureAsEndDevice(const TlvSet &aTypes);

    /**
     * Stops the diagnostic server and cleans up all associated resources.
     *
     * This method performs the following operations:
     * - Clears self-enabled and self-pending update flags
     * - Stops the update timer
     * - On FTD builds:
     *   -> Stops the registration timer
     *   -> Clears update pending flag
     *   -> Clears child and neighbor enabled flags
     *   -> Unregisters the client
     *   -> Resets diagnostic cache for all children
     *   -> Schedules child timer if device is a router or leader
     * - Deactivates the server
     *
     */
    void StopServer(void);

    /**
     * Sends a diagnostic update message to the parent as a child device.
     *
     * This method constructs and sends a confirmable POST message to the parent device with the
     * URI path for diagnostic end device updates. It includes self TLVs (Type-Length-Value) data
     * that has been marked as dirty (modified).
     *
     * @retval kErrorNone           Successfully sent the update message.
     * @retval kErrorInvalidState   The device is not currently in child state.
     * @retval kErrorAlready        An update has already been sent and is pending.
     * @retval kErrorNoBufs         Failed to allocate message buffer.
     * @retval kErrorOther          Other errors that may occur during message construction or sending.
     *
     * @note This method will only send an update if the device is in child state and no previous
     *       update is pending (mUpdateSent is false).
     * @note The method moves pending dirty flags to mSelfPendingUpdate and clears mSelfDirty upon
     *       successful message preparation.
     * @note If an error occurs, the message is automatically freed and an error is logged.
     */
    Error SendEndDeviceUpdate(void);

    /**
     * Handles the response from an End Device Update sent to the parent.
     *
     * This method processes the response received from the parent router after sending a End Device Update.
     * If the update was successful, it clears the pending update state. If the update failed, it merges the
     * pending update back into the dirty flags to ensure the update is retried later.
     *
     * @param[in] aResponse      A pointer to the CoAP response message (unused).
     * @param[in] aMessageInfo   A pointer to the message info associated with the response (unused).
     * @param[in] aResult        The result of the Child Update operation.
     *
     */
    void        HandleEndDeviceUpdateAck(Coap::Message *aResponse, const Ip6::MessageInfo *aMessageInfo, Error aResult);
    static void HandleEndDeviceUpdateAck(void                *aContext,
                                         otMessage           *aResponse,
                                         const otMessageInfo *aMessageInfo,
                                         otError              aResult);

#if OPENTHREAD_FTD
    /**
     * Sends a full server update (SU) to client device.
     *
     * This method constructs and sends a diagnostic server update (SU) message containing enabled diagnostic
     * information (self, child, and neighbor contexts) to the specified client address. The message is sent as a
     * confirmable CoAP POST to the diagnostic server update URI.
     *
     * @param[in] aClientAddr  The IPv6 address of the client to send the full server update to.
     *
     * @retval kErrorNone     Successfully sent the full server update.
     * @retval kErrorBusy     A pending update is awaiting acknowledgment.
     * @retval kErrorNoBufs   Insufficient buffers to allocate the message.
     * @retval kErrorFailed   Failed to append data to the message or send the message.
     *
     * @note If self diagnostics are enabled, the host context is appended to the message.
     * @note If child diagnostics are enabled, a child baseline update is scheduled.
     * @note If neighbor diagnostics are enabled, neighbor context queries for all valid routers are appended.
     * @note The method sets mUpdatePending flag when a message is successfully sent.
     */
    Error SendFullServerUpdate(const Ip6::Address &aClientAddr);

    /**
     * Sends an empty server update (SU) to a client device.
     *
     * This method sends an empty diagnostic server update (SU) message to the specified client address
     * using a confirmable CoAP POST request. The message includes routing information and
     * a sequence number.
     *
     * @param[in] aClientAddr  The IPv6 address of the client device to send the response to.
     *
     * @retval kErrorNone     Successfully sent the registration response.
     * @retval kErrorBusy     A previous update is still pending an acknowledgment.
     * @retval kErrorNoBufs   Failed to allocate a message buffer.
     * @retval kError*        Other errors from message preparation or transmission.
     *
     * @note The method will not send a new update if there is already a pending update
     *       waiting for acknowledgment.
     */
    Error SendEmptyServerUpdate(const Ip6::Address &aClientAddr);

    /**
     * Sends a diagnostic server update (SU) message to the diagnostic client.
     *
     * This method constructs and sends a diagnostic server update containing router information,
     * host TLVs, child updates, and neighbor updates. Updates are sent in batches if needed,
     * with child updates resuming from `mChildResumeIndex` to handle large datasets.
     *
     * The update process follows this sequence:
     * - First batch (startIndex == 0): Includes host TLVs for self-diagnostics
     * - Subsequent batches: Include remaining child updates
     * - Final batch: Includes neighbor updates (only when no more child updates pending)
     *
     * The method ensures only one update is in flight at a time by checking `mUpdatePending`.
     * On success, schedules another update if more data remains to be sent.
     *
     * @retval kErrorNone       Successfully sent the update message.
     * @retval kErrorBusy       A previous update is still pending acknowledgment.
     * @retval kErrorNoBufs     Insufficient buffer space to allocate the message.
     * @retval kErrorFailed     Failed to append child or neighbor updates to the message.
     *
     */
    Error SendServerUpdate(void);

    /**
     * Starts the diagnostic server as a router with specified TLV sets.
     *
     * This method initializes or updates the diagnostic server to operate in router mode,
     * configuring which diagnostic TLVs to track for the device itself, its children, and
     * its neighbors. If the server is not already active, it performs full initialization
     * including generating a new sequence number and resetting all tracking state.
     *
     * @param[in] aSelf      The set of TLVs to enable for self (this device) diagnostics.
     * @param[in] aChild     The set of TLVs to enable for child device diagnostics.
     * @param[in] aNeighbor  The set of TLVs to enable for neighbor device diagnostics.
     * @param[in] aQuery     Whether this is triggered by a query operation.
     *
     * @retval kErrorNone         Successfully started the server as router.
     * @retval kErrorInvalidArgs  Both aSelf and aChild TLV sets are empty when starting
     *                            an inactive server.
     *
     */
    Error ConfigureAsRouter(const TlvSet &aSelf, const TlvSet &aChild, const TlvSet &aNeighbor, bool aQuery);

    /**
     * Updates the diagnostic server state for all valid child devices.
     *
     * This method iterates through all children in a valid state and updates their
     * diagnostic server information based on device type and change flags.
     *
     * @param[in] aMtdChanged  Indicates whether MTD (Minimal Thread Device) configuration has changed.
     * @param[in] aFtdChanged  Indicates whether FTD (Full Thread Device) configuration has changed.
     * @param[in] aQuery       Indicates whether this is a query operation that should trigger an update
     *                         regardless of change flags.
     *
     * @note The method applies the appropriate change flag based on each child's device type:
     *       - FTD children use the aFtdChanged flag
     *       - MTD children use the aMtdChanged flag
     *       The change flag is combined with aQuery using logical OR before updating each child.
     */
    void SyncAllChildDiagStates(bool aMtdChanged, bool aFtdChanged, bool aQuery);

    /**
     * Updates the diagnostic server state for a child device.
     *
     * This method manages the diagnostic server state transitions for a child based on whether
     * diagnostic features are enabled and the child's current state. It determines the appropriate
     * TLV set based on whether the child is a Full Thread Device (FTD) or Minimal Thread Device (MTD),
     * and sends start or stop commands accordingly.
     *
     * @param[in] aChild  Reference to the child device to update.
     * @param[in] aQuery  Whether to force a query/refresh of the child's diagnostic state.
     *                    When true, active children will be re-queried, and unknown/stopped
     *                    children will be started with a query request.
     *
     * @note If the diagnostic TLV set is empty, the child's diagnostic cache is reset and
     *       a stop command is sent if the child is in an active state.
     * @note If the diagnostic TLV set is not empty, the method attempts to start or refresh
     *       the child's diagnostic server based on its current state.
     * @note Failed start operations will set the child's state to unknown, ensuring retry
     *       on the next update.
     */
    void SyncChildDiagState(Child &aChild, bool aQuery);

    /**
     * Sends a diagnostic end device request stop to a child device to stop its diagnostic server.
     *
     * This method sends a CoAP confirmable POST message to a child device to stop its diagnostic server.
     * If a previous diagnostic server command is pending for the child, it will be aborted before
     * sending the new stop command.
     *
     * @param[in] aChild  A reference to the child device to send the stop command to.
     *
     * @retval kErrorNone     Successfully sent the stop command.
     * @retval kErrorNoBufs   Insufficient buffers available to allocate the message.
     * @retval kError*        Other error codes as returned by message append or send operations.
     *
     */
    Error SendEndDeviceRequestStop(Child &aChild);

    /**
     * Sends a diagnostic end device request start to a child device to start its diagnostic server.
     *
     * This method initiates a diagnostic session with a specified child device by sending
     * a CoAP confirmable POST message to the diagnostic endpoint. If there's already a
     * pending diagnostic transaction for the child, it will be aborted before starting
     * a new one.
     *
     * @param[in] aChild  A reference to the child device to send the command to.
     * @param[in] aTypes  A set of TLV types to include in the diagnostic request.
     * @param[in] aQuery  A boolean indicating whether this is a query request.
     *
     * @retval kErrorNone     Successfully sent the diagnostic start command.
     * @retval kErrorNoBufs   Insufficient buffer space to create the message.
     * @retval other          Other errors from message append or send operations.
     *
     * @note The child's diagnostic server state is set to pending upon successful send.
     * @note Any previously pending diagnostic transaction for this child will be aborted.
     */
    Error SendEndDeviceRequestStart(Child &aChild, const TlvSet &aTypes, bool aQuery);

    /**
     * Sends a recovery diagnostic query to a child device to recover evicted cache data.
     *
     * This method constructs and sends a CoAP confirmable POST message to re-query diagnostic
     * TLVs that were previously evicted from the cache due to memory pressure.
     *
     * @param[in,out] aChild  Reference to the child device to query.
     * @param[in]     aTypes  Set of lost TLV types to recover from the child.
     *
     * @retval kErrorNone   Successfully sent the lost query message.
     * @retval kErrorNoBufs Insufficient buffer space to allocate the message.
     * @retval other        Other errors from message append or send operations.
     */
    Error SendEndDeviceRecoveryQuery(Child &aChild, const TlvSet &aTypes);

    /**
     * Handles the response to an End Device Request (ER) command sent to a child.
     *
     * This method processes the acknowledgment from a child device after sending an ER Start or Stop command.
     * It performs state transitions, updates the diagnostic cache, and handles failures by setting the child
     * to an unknown state for later retry.
     *
     * State transitions handled:
     * - ER Start success: ActivePending → Active
     * - ER Stop success: StopPending → Stopped
     * - Failure: Current state → Unknown (for retry)
     *
     * On successful ER Start ACK, the child's diagnostic cache is updated with TLVs from the response message.
     * On failure, the child's pending command timer is scheduled for retry via HandleChildTimer.
     *
     * @param[in,out] aChild         Reference to the child device that sent the response.
     * @param[in]     aResponse      Pointer to the CoAP response message (may be nullptr on failure).
     * @param[in]     aMessageInfo   Pointer to the message info (unused).
     * @param[in]     aResult        Result code of the ER command operation.
     *
     * @note If the response is successful and contains data, the child's diagnostic cache is updated.
     * @note On failure, the child state is set to Unknown to trigger retry logic in SyncChildDiagState.
     * @note The child timer is scheduled to retry failed operations.
     */
    void        HandleEndDeviceRequestAck(Child                  &aChild,
                                          Coap::Message          *aResponse,
                                          const Ip6::MessageInfo *aMessageInfo,
                                          Error                   aResult);
    static void HandleEndDeviceRequestAck(void                *aContext,
                                          otMessage           *aResponse,
                                          const otMessageInfo *aMessageInfo,
                                          otError              aResult);

    /**
     * Handles the response to a End Device Request (ER) recovery query sent to a child.
     *
     * This method processes the response from a child device after sending a ER recovery query to recover
     * diagnostic TLVs that were evicted from the cache. On success, it clears the lost query pending flag
     * and updates the child's diagnostic cache with the recovered TLVs.
     *
     * Lost queries are used when the router's cache buffer limit is exceeded and child cache entries are evicted.
     * This mechanism allows the router to recover the evicted data from the child on demand.
     *
     * @param[in,out] aChild         Reference to the child device that sent the response.
     * @param[in]     aResponse      Pointer to the CoAP response message (may be nullptr on failure).
     * @param[in]     aMessageInfo   Pointer to the message info (unused).
     * @param[in]     aResult        Result code of the lost query operation.
     *
     * @note On success, clears the child's lost query pending flag.
     * @note On success, updates the child's diagnostic cache with recovered TLVs from the response.
     * @note On failure, the lost query pending flag remains set, and HandleChildTimer will retry.
     */
    void        HandleEndDeviceRecoveryAck(Child                  &aChild,
                                           Coap::Message          *aResponse,
                                           const Ip6::MessageInfo *aMessageInfo,
                                           Error                   aResult);
    static void HandleEndDeviceRecoveryAck(void                *aContext,
                                           otMessage           *aResponse,
                                           const otMessageInfo *aMessageInfo,
                                           otError              aResult);

    /**
     * Handles the response to a Server Update (SU) message sent to a client.
     *
     * This method processes acknowledgments from diagnostic clients after sending an SU message.
     * It implements exponential backoff for retry logic and maintains at-most-once semantics by
     * incrementing the sequence number when retries are exhausted.
     *
     * Retry behavior:
     * - Success: Clears retry count, increments sequence number, schedules next batch if pending
     * - Failure: Increments retry count, applies exponential backoff (10s, 20s, 40s, 80s, 160s, capped at 320s max)
     * - After 5 failures: Gives up, increments sequence to maintain at-most-once semantics
     *
     * At-most-once semantics ensure clients never process duplicate SU messages even if ACKs are lost.
     * When a router gives up after 5 retries, incrementing the sequence guarantees the next SU uses
     * a new sequence number, preventing clients from treating it as a retransmission.
     *
     * @param[in] aResponse      Pointer to the CoAP response message (unused).
     * @param[in] aMessageInfo   Pointer to the message info (unused).
     * @param[in] aResult        Result code of the SU operation.
     *
     * @note On success: mSequenceNumber++, mUpdateRetryCount = 0, mUpdatePending = false
     * @note On failure: mUpdateRetryCount++, exponential backoff applied
     * @note After 5 retries: mSequenceNumber++ (at-most-once semantics), mUpdatePending = false
     * @note Schedules next SU batch if mChildResumeIndex > 0 (more children to send)
     */
    void HandleServerUpdateAck(Coap::Message *aResponse, const Ip6::MessageInfo *aMessageInfo, Error aResult);

    static void HandleServerUpdateAck(void                *aContext,
                                      otMessage           *aResponse,
                                      const otMessageInfo *aMessageInfo,
                                      otError              aResult);
#endif

    /**
     * Appends host diagnostic TLVs to a CoAP message.
     *
     * This method iterates through the specified TLV set and appends the corresponding diagnostic
     * data for this device (the host router or end device) to the message. It handles various TLV
     * types including hardware identifiers, network statistics, and addressing information.
     *
     * Supported TLV types include:
     * - Hardware: MAC address, EUI-64, vendor name/model/version
     * - Network: Mode, timeout, ML-EID, IP addresses, ALOCs
     * - Statistics: MAC counters, MLE counters, link error rates, link margin
     * - Version info: Thread spec version, stack version
     *
     * @param[in,out] aMessage  Reference to the CoAP message to append TLVs to.
     * @param[in]     aTlvs     Set of TLV types to append.
     *
     * @retval kErrorNone     Successfully appended all requested TLVs.
     * @retval kErrorNoBufs   Insufficient buffer space in the message.
     * @retval kErrorFailed   Failed to retrieve or format a specific TLV.
     *
     * @note Only TLVs present in aTlvs will be appended.
     * @note If appending any TLV fails, the method returns immediately with an error.
     */
    Error AppendHostTlvs(Message &aMessage, TlvSet aTlvs);
#if OPENTHREAD_FTD
    /**
     * Appends a host context to a Server Update (SU) message.
     *
     * This method constructs a host context header and appends self TLVs representing the router's
     * own diagnostic information. The context is prepended with a Context structure indicating the
     * device type (host) and the total length of the context including all TLVs.
     *
     * Structure:
     * - Context header: Type=kTypeHost, Length=<total>
     * - Self TLVs: As specified by aTlvs parameter
     *
     * @param[in,out] aMessage  Reference to the message to append the host context to.
     * @param[in]     aTlvs     Set of TLV types to include in the host context.
     *
     * @retval kErrorNone     Successfully appended the host context.
     * @retval kErrorNoBufs   Insufficient buffer space in the message.
     * @retval kErrorFailed   Failed to append self TLVs.
     *
     * @note The context header is written at the start, then updated with the final length after TLVs are appended.
     */
    Error AppendHostContext(Message &aMessage, TlvSet aTlvs);

    /**
     * Appends a child context to a Server Update (SU) message.
     *
     * This method constructs a child context containing the child's current diagnostic state,
     * including both host-provided TLVs (from the router) and child-provided TLVs (from the child's cache).
     * The update mode indicates whether this is an addition, removal, or update.
     *
     * Update modes:
     * - kModeAdded: Child newly attached or first SU after client registration (includes all enabled TLVs)
     * - kModeUpdate: Child still attached, only dirty TLVs included
     * - kModeRemove: Child detached, only child ID included (no TLVs)
     *
     * The method only appends a context if the child has pending updates (ShouldSendDiagUpdate returns true)
     * or if the child's attach state is dirty (newly attached or detached).
     *
     * Structure:
     * - ChildContext header: Type=kTypeChild, Id=<child ID>, UpdateMode=<mode>, Length=<total>
     * - Host-provided TLVs: Link metrics, timeout, mode (if dirty or kModeAdded)
     * - Child-provided TLVs: From child's diagnostic cache (if valid)
     *
     * @param[in,out] aMessage  Reference to the message to append the context to.
     * @param[in,out] aChild    Reference to the child device to create a context for.
     *
     * @retval kErrorNone     Successfully appended the child context update.
     * @retval kErrorNoBufs   Insufficient buffer space in the message.
     *
     * @note Returns success immediately (with no append) if child has no pending updates.
     * @note For removed children (detached), only the context header is appended (no TLVs).
     * @note For added/updated children, both host-provided and cached child-provided TLVs are included.
     */
    Error AppendChildContext(Message &aMessage, Child &aChild);

    /**
     * Appends child diagnostic TLVs to a message.
     *
     * This method appends host-provided diagnostic TLVs about a child device to the message.
     * These are TLVs that the router tracks about the child, such as link quality metrics,
     * timeouts, addressing, and device properties.
     *
     * Host-provided child TLVs include:
     * - MAC address, Mode, Timeout, Last Heard, Connection Time
     * - CSL parameters (channel, timeout, period)
     * - Link quality: Link Margin In, MAC Link Error Rates Out
     * - FTD-only: ML-EID, IP address list, ALOC list
     * - Thread Spec Version
     *
     * @param[in,out] aMessage  Reference to the message to append TLVs to.
     * @param[in]     aTlvs     Set of TLV types to append.
     * @param[in]     aChild    Reference to the child device to retrieve TLV data from.
     *
     * @retval kErrorNone     Successfully appended all requested TLVs.
     * @retval kErrorNoBufs   Insufficient buffer space in the message.
     *
     * @note Only TLVs present in both aTlvs and the host-provided valid mask are appended.
     * @note IP address list and ALOC list are only appended for FTD children.
     */
    Error AppendChildTlvs(Message &aMessage, TlvSet aTlvs, const Child &aChild);

    /**
     * Appends a neighbor context baseline to a message.
     *
     * This method constructs a neighbor context for a router neighbor, including diagnostic TLVs
     * that describe the neighbor's link quality and properties. The context is marked with kModeAdded
     * to indicate this is a baseline query for all enabled neighbor TLVs.
     *
     * Structure:
     * - NeighborContext header: Type=kTypeNeighbor, Id=<router ID>, UpdateMode=kModeAdded, Length=<total>
     * - Neighbor TLVs: All enabled neighbor TLVs (link metrics, connection time, MAC address, etc.)
     *
     * @param[in,out] aMessage  Reference to the message to append the context to.
     * @param[in]     aRouter   Reference to the router neighbor to create a context for.
     *
     * @retval kErrorNone     Successfully appended the neighbor context baseline.
     * @retval kErrorNoBufs   Insufficient buffer space in the message.
     *
     * @note This is used when initially querying neighbor information (e.g., on client registration).
     */
    Error AppendNeighborContextBaseline(Message &aMessage, const Router &aRouter);

    /**
     * Appends a neighbor context update to a Server Update (SU) message.
     *
     * This method constructs a neighbor context update for a router neighbor. The update mode depends
     * on the neighbor's current state and whether it has been previously reported to clients:
     * - kModeAdded: Neighbor newly appeared (router state bit set in mRouterStateMask)
     * - kModeUpdate: Neighbor still valid, dynamic TLVs may have changed
     * - kModeRemove: Neighbor is no longer valid (router table entry removed)
     *
     * For kModeUpdate, only dynamic (non-static) TLVs are included to reduce message size.
     * Static TLVs (MAC address, Connection Time, Thread Spec Version) are omitted in updates.
     *
     * Structure:
     * - NeighborContext header: Type=kTypeNeighbor, Id=<router ID>, UpdateMode=<mode>, Length=<total>
     * - Neighbor TLVs: All enabled TLVs (Added), or dynamic TLVs only (Update), or none (Remove)
     *
     * @param[in,out] aMessage  Reference to the message to append the context to.
     * @param[in]     aId       Router ID of the neighbor to create a context for.
     *
     * @retval kErrorNone     Successfully appended the neighbor context update.
     * @retval kErrorNoBufs   Insufficient buffer space in the message.
     *
     * @note The method skips neighbors that haven't changed (neither valid nor in mRouterStateMask).
     * @note For kModeUpdate, static TLVs are filtered out to send only dynamic link metrics.
     */
    Error AppendNeighborContextUpdate(Message &aMessage, uint8_t aId);

    /**
     * Appends neighbor diagnostic TLVs to a message.
     *
     * This method appends diagnostic TLVs for a router neighbor to the message. These TLVs describe
     * the link quality and properties of the neighbor relationship from this router's perspective.
     *
     * Supported neighbor TLVs:
     * - MAC address (Extended Address)
     * - Last Heard (time since last frame received)
     * - Connection Time (duration as neighbor)
     * - Link Margin In (inbound link quality metrics: margin, average RSSI, last RSSI)
     * - MAC Link Error Rates Out (outbound message/frame error rates)
     * - Thread Spec Version
     *
     * @param[in,out] aMessage   Reference to the message to append TLVs to.
     * @param[in]     aTlvs      Set of TLV types to append.
     * @param[in]     aNeighbor  Reference to the router neighbor to retrieve TLV data from.
     *
     * @retval kErrorNone     Successfully appended all requested TLVs.
     * @retval kErrorNoBufs   Insufficient buffer space in the message.
     *
     * @note Only TLVs present in both aTlvs and the neighbor-valid mask are appended.
     */
    Error AppendNeighborTlvs(Message &aMessage, TlvSet aTlvs, const Router &aNeighbor);

    /**
     * Appends child context updates to a Server Update (SU) message, respecting MTU limits.
     *
     * This method iterates through all children (or a subset when resuming) and appends their context updates
     * to the message. It implements batching to handle large networks: if the message exceeds the MTU limit
     * (kMaxUpdateMessageLength = 1232 bytes), it stops appending and sets aNeedsMore=true, saving the resume
     * index in mChildResumeIndex.
     *
     * Batching behavior:
     * - Starts iteration from mChildResumeIndex (0 for first batch, >0 for continuation)
     * - Stops when message length exceeds kMaxUpdateMessageLength
     * - Sets aNeedsMore=true and saves mChildResumeIndex for next batch
     * - On completion, resets mChildResumeIndex=0 and aNeedsMore=false
     *
     * The method supports two modes:
     * - Baseline mode (mSendChildBaseline=true): Iterates only valid children, marks all as dirty (added)
     * - Update mode: Iterates all children, appends only those with pending updates
     *
     * @param[in,out] aMessage    Reference to the message to append child updates to.
     * @param[out]    aNeedsMore  Set to true if more children remain to be sent in a subsequent batch.
     *
     * @retval true   Successfully appended all requested child updates (or batch completed).
     * @retval false  Failed to append a child context update due to an error other than buffer overflow.
     *
     * @note Returns false only on errors other than kErrorNoBufs (which triggers batching).
     * @note When batching (aNeedsMore=true), the current child's update is aborted and will retry in next batch.
     * @note Clears mSendChildBaseline flag on completion of baseline iteration.
     */
    bool AppendChildContextBatch(Message &aMessage, bool &aNeedsMore);
    /**
     * Appends neighbor context updates to a Server Update (SU) message, respecting MTU limits.
     *
     * This method iterates through all router IDs (0 to kMaxRouterId-1) and appends neighbor context
     * updates for routers that have changed state. It stops appending if the message exceeds the MTU
     * limit (kMaxUpdateMessageLength = 1232 bytes).
     *
     * Unlike child updates, neighbor updates do not implement full batching with resume indices.
     * If the message fills up, the method stops and returns success, leaving remaining neighbor
     * updates for the next SU cycle.
     *
     * @param[in,out] aMessage  Reference to the message to append neighbor updates to.
     *
     * @retval true   Successfully appended all attempted neighbor updates (may be partial due to MTU).
     * @retval false  Failed to append a neighbor context update due to an error other than buffer overflow.
     *
     * @note Neighbor updates are always attempted in the final batch (after all child updates complete).
     * @note If MTU is exceeded, remaining neighbors are deferred to the next update cycle.
     * @note The method updates mRouterStateMask as neighbors transition from "new" to "known" state.
     */
    bool AppendNeighborContextBatch(Message &aMessage);

    /**
     * Begins a diagnostic update transaction for all children.
     *
     * This method locks the diagnostic cache for all children by calling LockChildCaches on each child.
     * The locked cache ensures that child TLVs are consistent during the construction of a Server Update
     * (SU) message, even if the SU spans multiple batches due to MTU limits.
     *
     * The cache remains locked until CommitChildCacheUpdates (on success) or UnlockChildCaches (on failure/timeout).
     *
     * @note Called at the start of SendServerUpdate before appending child contexts.
     * @note Locks the cache for all children (kInStateAny), not just valid children.
     */
    void LockChildCaches(void);

    /**
     * Commits the diagnostic update transaction for all children.
     *
     * This method commits the locked diagnostic cache for all children, clearing dirty flags and
     * unlocking the cache. It is called after a successful Server Update (SU) message is sent and
     * acknowledged by the client.
     *
     * Only children whose cache is still locked (not aborted) will have their updates committed.
     * This ensures that children whose contexts failed to append (e.g., due to MTU limits) do not
     * lose their dirty state.
     *
     * @note Called in HandleServerUpdateAck after successful ACK from client.
     * @note Only commits children with locked caches (skips aborted children).
     */
    void CommitChildCacheUpdates(void);

    /**
     * Aborts the diagnostic update transaction for all children.
     *
     * This method aborts the locked diagnostic cache for all valid children, restoring dirty flags
     * and unlocking the cache. It is called when a Server Update (SU) message fails to send or times
     * out waiting for acknowledgment.
     *
     * Aborting ensures that dirty TLVs are preserved for retry in the next update cycle.
     *
     * @note Called when SendServerUpdate fails, or when HandleServerUpdateAck receives an error.
     * @note Only aborts children in valid state (children that were included in the attempted SU).
     */
    void UnlockChildCaches(void);
#endif

    /**
     * Filters an IPv6 address to determine if it should be included in diagnostic reports.
     *
     * This filter excludes addresses that are considered internal or not useful for diagnostic purposes:
     * - Mesh-local addresses (Thread ML-EID is reported separately via kMlEid TLV)
     * - Link-local unicast/multicast addresses (reported separately via kIp6LinkLocalAddressList)
     * - Realm-local all-nodes multicast (ff03::1)
     * - Realm-local all-routers multicast (ff03::2)
     * - Realm-local all-MPL-forwarders multicast (ff03::fc)
     * - Anycast locator addresses (ALOCs, reported separately via kAlocList)
     *
     * @param[in] aAddress  The IPv6 address to filter.
     *
     * @retval true   The address should be included in the kIp6AddressList TLV.
     * @retval false  The address should be excluded (matches one of the filter criteria).
     *
     * @note This is used when constructing kIp6AddressList TLVs for both self and children.
     */
    bool ShouldIncludeIp6Address(const Ip6::Address &aAddress);

    /**
     * Filters an IPv6 address to determine if it is an ALOC (Anycast Locator).
     *
     * This method checks if the address has an anycast locator IID. If so, it extracts the ALOC
     * value (the locator field) and returns true.
     *
     * @param[in]  aAddress  The IPv6 address to filter.
     * @param[out] aAloc     The ALOC value (locator byte) if the address is an ALOC.
     *
     * @retval true   The address is an ALOC, and aAloc contains the locator value.
     * @retval false  The address is not an ALOC.
     *
     * @note Used when constructing kAlocList TLVs for both self and children.
     */
    bool ShouldIncludeAloc(const Ip6::Address &aAddress, uint8_t &aAloc);

    /**
     * Filters an IPv6 address to determine if it is a link-local address suitable for diagnostic reports.
     *
     * This filter includes link-local unicast and multicast addresses, but excludes well-known
     * link-local multicast addresses:
     * - Link-local all-nodes multicast (ff02::1)
     * - Link-local all-routers multicast (ff02::2)
     *
     * @param[in] aAddress  The IPv6 address to filter.
     *
     * @retval true   The address is a link-local address and should be included in kIp6LinkLocalAddressList.
     * @retval false  The address should be excluded (not link-local, or a well-known multicast address).
     *
     * @note Used when constructing kIp6LinkLocalAddressList TLVs for self.
     */
    bool ShouldIncludeLinkLocalAddress(const Ip6::Address &aAddress);

    /**
     * Appends the kIp6AddressList TLV for this device to a message.
     *
     * This method iterates through all unicast and multicast addresses assigned to this device,
     * filters them using ShouldIncludeIp6Address, and appends the filtered addresses as a single
     * kIp6AddressList TLV. The TLV uses either Tlv or ExtendedTlv format depending on the number
     * of addresses.
     *
     * @param[in,out] aMessage  Reference to the message to append the TLV to.
     *
     * @retval kErrorNone     Successfully appended the kIp6AddressList TLV.
     * @retval kErrorNoBufs   Insufficient buffer space in the message.
     *
     * @note Excludes mesh-local, link-local, anycast, and well-known multicast addresses (see ShouldIncludeIp6Address).
     * @note Uses ExtendedTlv if the address count exceeds Tlv::kBaseTlvMaxLength (255 bytes / 16 = 15 addresses).
     */
    Error AppendHostIp6AddressList(Message &aMessage);

    /**
     * Appends the kAlocList TLV for this device to a message.
     *
     * This method iterates through all unicast addresses assigned to this device, filters them
     * using ShouldIncludeAloc to identify ALOCs, and appends the ALOC values (locator bytes) as a single
     * kAlocList TLV. The TLV uses either Tlv or ExtendedTlv format depending on the number of ALOCs.
     *
     * @param[in,out] aMessage  Reference to the message to append the TLV to.
     *
     * @retval kErrorNone     Successfully appended the kAlocList TLV.
     * @retval kErrorNoBufs   Insufficient buffer space in the message.
     *
     * @note Each ALOC is represented as a single byte (the locator value from the IID).
     * @note Uses ExtendedTlv if the ALOC count exceeds Tlv::kBaseTlvMaxLength (255).
     */
    Error AppendHostAlocList(Message &aMessage);

    /**
     * Appends the kIp6LinkLocalAddressList TLV for this device to a message.
     *
     * This method iterates through all unicast and multicast addresses assigned to this device,
     * filters them using ShouldIncludeLinkLocalAddress, and appends the filtered link-local addresses
     * as a single kIp6LinkLocalAddressList TLV. The TLV uses either Tlv or ExtendedTlv format
     * depending on the number of addresses.
     *
     * @param[in,out] aMessage  Reference to the message to append the TLV to.
     *
     * @retval kErrorNone     Successfully appended the kIp6LinkLocalAddressList TLV.
     * @retval kErrorNoBufs   Insufficient buffer space in the message.
     *
     * @note Excludes well-known link-local multicast addresses (see ShouldIncludeLinkLocalAddress).
     * @note Uses ExtendedTlv if the address count exceeds Tlv::kBaseTlvMaxLength (255 bytes / 16 = 15 addresses).
     */
    Error AppendHostLinkLocalAddressList(Message &aMessage);

#if OPENTHREAD_FTD
    /**
     * Appends the kIp6AddressList TLV for a child device to a message.
     *
     * This method iterates through all IPv6 addresses registered by the child, filters them using
     * ShouldIncludeIp6Address, and appends the filtered addresses as a single kIp6AddressList TLV.
     * The TLV uses either Tlv or ExtendedTlv format depending on the number of addresses.
     *
     * @param[in,out] aMessage  Reference to the message to append the TLV to.
     * @param[in]     aChild    Reference to the child device to retrieve addresses from.
     *
     * @retval kErrorNone     Successfully appended the kIp6AddressList TLV.
     * @retval kErrorNoBufs   Insufficient buffer space in the message.
     *
     * @note Excludes mesh-local, link-local, anycast, and well-known multicast addresses (see ShouldIncludeIp6Address).
     * @note Uses ExtendedTlv if the address count exceeds Tlv::kBaseTlvMaxLength (255 bytes / 16 = 15 addresses).
     */
    Error AppendChildIp6AddressList(Message &aMessage, const Child &aChild);

    /**
     * Appends the kAlocList TLV for a child device to a message.
     *
     * This method iterates through all IPv6 addresses registered by the child, filters them using
     * ShouldIncludeAloc to identify ALOCs, and appends the ALOC values (locator bytes) as a single
     * kAlocList TLV. The TLV uses either Tlv or ExtendedTlv format depending on the number of ALOCs.
     *
     * @param[in,out] aMessage  Reference to the message to append the TLV to.
     * @param[in]     aChild    Reference to the child device to retrieve ALOCs from.
     *
     * @retval kErrorNone     Successfully appended the kAlocList TLV.
     * @retval kErrorNoBufs   Insufficient buffer space in the message.
     *
     * @note Each ALOC is represented as a single byte (the locator value from the IID).
     * @note Uses ExtendedTlv if the ALOC count exceeds Tlv::kBaseTlvMaxLength (255).
     */
    Error AppendChildAlocList(Message &aMessage, const Child &aChild);
#endif

    /**
     * Schedules the diagnostic update timer to fire after the specified delay.
     *
     * This method schedules the update timer to fire at the specified delay from now. If the timer
     * is already scheduled to fire earlier, it is not rescheduled (FireAtIfEarlier ensures the
     * earliest deadline is preserved).
     *
     * The update timer triggers sending diagnostic updates (SU messages on routers, EU messages on children).
     *
     * @param[in] aDelay  Delay in milliseconds before the timer should fire.
     *
     * @note Uses FireAtIfEarlier to avoid delaying already-scheduled updates.
     */
    void ScheduleUpdateTimer(uint32_t aDelay) { mUpdateTimer.FireAtIfEarlier(TimerMilli::GetNow() + aDelay); }

    /**
     * Handles the diagnostic update timer firing.
     *
     * This method is called when the update timer fires. It determines the device's role and sends
     * the appropriate diagnostic update:
     * - Router/Leader: Sends Server Update (SU) via SendServerUpdate
     * - Child: Sends End Device Update (EU) via SendEndDeviceUpdate
     *
     * On successful send, the method updates mSelfDirty to only include extended-delay TLVs and
     * schedules the next update timer if any extended-delay TLVs remain (or if child/neighbor
     * extended-delay TLVs exist on routers).
     *
     * On failure, schedules a retry after kUpdateBaseDelay (1 second).
     *
     * @note Extended-delay TLVs (e.g., counters, link metrics) are sent less frequently than
     *       base-delay TLVs to reduce network traffic.
     */
    void HandleUpdateTimer(void);

#if OPENTHREAD_FTD
    /**
     * Checks if the total child diagnostic cache usage exceeds the configured limit.
     *
     * This method sums the cache buffer usage across all valid children. If the total exceeds
     * kCacheBuffersLimit (default 40 buffers), it schedules an immediate update to allow the
     * router to send cached child data to clients, freeing up buffer space.
     *
     * Cache eviction occurs when the limit is exceeded: the least-recently-updated child's cache
     * is evicted. This method proactively triggers updates to avoid excessive evictions.
     *
     * @note Called after appending child cache data to messages (e.g., in HandleEndDeviceRequestAck).
     * @note Schedules update timer with 0 delay if limit is exceeded.
     */
    void UpdateIfCacheBufferLimit(void);

    /**
     * Schedules the child timer to fire after kChildUpdateDelay (1 second).
     *
     * This method schedules the child timer to fire at kChildUpdateDelay from now. If the timer
     * is already scheduled to fire earlier, it is not rescheduled (FireAtIfEarlier ensures the
     * earliest deadline is preserved).
     *
     * The child timer triggers periodic checks of child diagnostic server states, including:
     * - Retrying failed ER Start/Stop commands (children in Unknown state)
     * - Sending lost TLV queries for children with evicted cache data
     *
     * @note Uses FireAtIfEarlier to avoid delaying already-scheduled child operations.
     */
    void ScheduleChildTimer(void) { mChildTimer.FireAtIfEarlier(TimerMilli::GetNow() + kChildUpdateDelay); }

    /**
     * Handles the child timer firing.
     *
     * This method is called when the child timer fires. It iterates through all valid children and:
     * 1. Calls SyncChildDiagState to retry failed ER commands or refresh active children
     * 2. Sends lost TLV queries for children with evicted cache data (ShouldSendLostDiagQuery)
     *
     * Lost TLV queries recover diagnostic data that was evicted from the router's cache due to
     * buffer limits. This ensures clients eventually receive all requested TLVs even if temporary
     * cache pressure caused evictions.
     *
     * @note Only operates when the device is a router or leader.
     * @note SyncChildDiagState handles state transitions (Unknown→ActivePending, etc.).
     * @note Lost queries are best-effort and may be skipped if message buffers are unavailable.
     */
    void HandleChildTimer(void);

    /**
     * Handles the registration timer firing.
     *
     * This method is called when the registration timer fires (every kRegistrationInterval).
     * It checks if any clients have registered during the current interval:
     * - If no clients registered: Stops the diagnostic server (no clients are interested)
     * - If clients registered: Resets the registration flag for the next interval and continues
     *
     * The method also checks if the requested TLV sets have changed (MTD/FTD child-provided TLVs).
     * If so, it calls SyncAllChildDiagStates to propagate the new TLV subscriptions to all children.
     *
     * Finally, if both mSelfEnabled and mChildEnabled are empty, the server is stopped.
     *
     * @note Only operates when the device is a router or leader and the server is active.
     * @note Client registration is signaled via HandleTmf<kUriExtDiagnosticServerRequest>.
     * @note SyncAllChildDiagStates is called with change flags to avoid unnecessary ER messages.
     */
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
    bool     mSendChildBaseline : 1; ///< When set, next SU will include baseline host-provided child TLVs
    bool     mUpdatePending : 1;
    uint16_t mChildResumeIndex; ///< Index to resume child iteration from after size-limited SU

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

#if OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_SERVER_ENABLE
DeclareTmfHandler(Server, kUriExtDiagnosticEndDeviceRequest);

#if OPENTHREAD_FTD
DeclareTmfHandler(Server, kUriExtDiagnosticEndDeviceUpdate);
DeclareTmfHandler(Server, kUriExtDiagnosticServerRequest);
#endif // OPENTHREAD_FTD

#endif // OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_SERVER_ENABLE

/**
 * Implements Extended Network Diagnostic client functionality.
 */
#if OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_ENABLE
class Client : public InstanceLocator, private NonCopyable
{
    friend class Tmf::Agent;

    static constexpr uint32_t kRegistrationInterval =
        OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_REGISTRATION_INTERVAL -
        (OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_REGISTRATION_JITTER *
         OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_REGISTRATION_AHEAD);
    static constexpr uint32_t kRegistrationJitter = OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_REGISTRATION_JITTER;

public:
    explicit Client(Instance &aInstance);

    /**
     * Starts the diagnostic client and requests the provided TLVs from
     * all servers.
     *
     * @param[in]  aHost      The TLVs requested for Host Contexts. Can be nullptr.
     * @param[in]  aChild     The TLVs requested for Child Contexts. Can be nullptr.
     * @param[in]  aNeighbor  The TLVs requested for Neighbor Contexts. Can be nullptr.
     */
    void Start(const TlvSet                              *aHost,
               const TlvSet                              *aChild,
               const TlvSet                              *aNeighbor,
               otExtNetworkDiagnosticServerUpdateCallback aCallback,
               void                                      *aContext);

    /**
     * Stops the diagnostic client.
     *
     * Any registered callback will immediately stop receiving updates until
     * they are explicitly re-registered with another call to `Start`.
     */
    void Stop(void);

    /**
     * Implements `otExtNetworkDiagnosticGetNextContext`.
     */
    static Error GetNextContext(const Coap::Message            &aMessage,
                                otExtNetworkDiagnosticIterator &aIterator,
                                otExtNetworkDiagnosticContext  &aContext);

    /**
     * Implements `otExtNetworkDiagnosticGetNextTlv`.
     */
    static Error GetNextTlv(const Coap::Message           &aMessage,
                            otExtNetworkDiagnosticContext &aContext,
                            otExtNetworkDiagnosticTlv     &aTlv);

    /**
     * Implements `otExtNetworkDiagnosticGetIp6Addresses`.
     */
    static Error GetIp6Addresses(const Coap::Message &aMessage,
                                 uint16_t             aDataOffset,
                                 uint16_t             aCount,
                                 otIp6Address        *aAddresses);

    /**
     * Implements `otExtNetworkDiagnosticGetAlocs`.
     */
    static Error GetAlocs(const Coap::Message &aMessage, uint16_t aDataOffset, uint16_t aCount, uint8_t *aAlocs);

private:
    /**
     * Sends a diagnostic server registration message to all routers.
     *
     * This method constructs and sends a non-confirmable CoAP POST message to the realm-local
     * all-routers multicast address (ff03::2) to register the client's diagnostic TLV subscriptions.
     * The registration includes the requested TLV sets for host, child, and neighbor contexts.
     *
     * @param[in] aQuery  If true, requests routers to immediately send a complete diagnostic update (query).
     *                    If false, requests routers to only send incremental updates going forward.
     *
     * @retval kErrorNone     Successfully sent the registration message.
     * @retval kErrorNoBufs   Insufficient buffer space to allocate the message.
     *
     * @note Registration messages are sent periodically (every kRegistrationInterval) to maintain subscriptions.
     * @note Routers will stop sending updates if no registration is received within kRegistrationInterval.
     */
    Error SendServerRequest(bool aQuery);

    /**
     * Sends a server request to a specific router to request a full diagnostic update for recovery.
     *
     * This method is called when the client detects a sequence number error (missed SU messages) from a
     * specific router. It sends a unicast non-confirmable CoAP POST message to the router to request a
     * complete diagnostic refresh.
     *
     * @param[in] aRloc16  The RLOC16 of the router to send the error query to.
     *
     * @retval kErrorNone     Successfully sent the error query message.
     * @retval kErrorNoBufs   Insufficient buffer space to allocate the message.
     *
     * @note Error queries do not include neighbor TLV requests (aIncludeNeighbors=false).
     * @note The router will respond with a complete SU message (complete flag set).
     */
    Error SendServerRequestForRecovery(uint16_t aRloc16);

    /**
     * Appends the server request payload to a CoAP message.
     *
     * This method constructs the payload for a diagnostic server registration request (SR) message.
     * The payload includes:
     * - RequestHeader: Flags for query and registration bits
     * - RequestContext structures: One for each non-empty TLV set (host, child, neighbor)
     *
     * @param[in,out] aMessage           Reference to the message to append the payload to.
     * @param[in]     aQuery             If true, sets the query flag in the request header.
     * @param[in]     aIncludeNeighbors  If true, includes neighbor TLV requests in the payload.
     *
     * @retval kErrorNone     Successfully appended the request payload.
     * @retval kErrorNoBufs   Insufficient buffer space in the message.
     *
     * @note aIncludeNeighbors is set to false for error queries to reduce message size.
     */
    Error AppendServerRequestPayload(Message &aMessage, bool aQuery, bool aIncludeNeighbors);

    /**
     * Appends a request context to a CoAP message.
     *
     * This method constructs a RequestContext structure for a specific device type (host, child, or neighbor)
     * and appends it to the message along with the requested TLV set.
     *
     * Structure:
     * - RequestContext header: Type, RequestSetCount, Length
     * - TLV set: Bitmask of requested TLV types (32 bytes)
     *
     * @param[in,out] aMessage  Reference to the message to append the context to.
     * @param[in]     aType     The device type (kTypeHost, kTypeChild, or kTypeNeighbor).
     * @param[in]     aSet      The TLV set to request for this device type.
     *
     * @retval kErrorNone     Successfully appended the request context.
     * @retval kErrorNoBufs   Insufficient buffer space in the message.
     */
    Error AppendRequestContext(Message &aMessage, DeviceType aType, const TlvSet &aSet);

    /**
     * Handles a TMF (Thread Management Framework) request for a specific URI.
     *
     * This template method processes incoming CoAP messages for diagnostic purposes
     * based on the specified URI template parameter.
     *
     * @tparam kUri         The URI that this handler is associated with.
     * @param[in] aMessage      A reference to the received CoAP message.
     * @param[in] aMessageInfo  A reference to the IPv6 message information containing
     *                          source and destination addresses.
     *
     */
    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * Processes a diagnostic server update (SU) message from a router.
     *
     * This method parses the UpdateHeader, validates the sequence number, and invokes the user callback
     * with the update message. It implements sequence number tracking to detect missed updates and
     * trigger error queries for full refreshes.
     *
     * Sequence number handling:
     * - Complete updates: Store the full sequence number (ignore previous sequence)
     * - Incremental updates: Validate expected sequence (previous + 1)
     * - Sequence error: Send error query to router to request full refresh
     *
     * On the first update from any router, mQueryPending is cleared to indicate successful registration.
     *
     * @param[in] aMessage      Reference to the received CoAP message.
     * @param[in] aMessageInfo  Reference to the message info containing source address.
     *
     * @note Maintains per-router sequence numbers in mServerSeqNumbers[routerId].
     * @note Invokes mCallback with the message, router RLOC16, and complete flag.
     */
    void ProcessServerUpdate(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * Schedules the next client registration timer to fire after the registration interval.
     *
     * This method schedules the registration timer to fire after kRegistrationInterval with a random
     * jitter of +/- kRegistrationJitter. The jitter helps distribute client registrations across
     * time to avoid network congestion.
     */
    void ScheduleNextRegistration(void);

    /**
     * Schedules a retry of the registration message after a short delay.
     *
     * This method schedules the registration timer to fire after a random delay between 0 and
     * kRegistrationJitter/5. This is used when SendServerRequest fails due to buffer exhaustion
     * or other transient errors.
     */
    void ScheduleRegistrationRetry(void);

    /**
     * Handles the client registration timer firing.
     *
     * This method is called when the registration timer fires. It attempts to send a registration message:
     * - If mQueryPending is true: Sends registration with query flag (requests full update)
     * - If mQueryPending is false: Sends registration without query (maintains subscription)
     *
     * On success, schedules the next registration timer via ScheduleNextRegistration.
     * On failure (e.g., kErrorNoBufs), schedules a retry via ScheduleRegistrationRetry.
     *
     * @note mQueryPending is cleared when the first SU is received from any router.
     * @note Only operates when the client is active (mActive=true).
     */
    void HandleRegistrationTimer(void);

    using UpdateTimer = TimerMilliIn<Client, &Client::HandleRegistrationTimer>;

    bool mActive : 1;
    bool mQueryPending : 1;

    TlvSet mHostSet;
    TlvSet mChildSet;
    TlvSet mNeighborSet;

    uint64_t mServerSeqNumbers[Mle::kMaxRouterId + 1]; ///< The last received sequence number from a server

    UpdateTimer mTimer;

    otExtNetworkDiagnosticServerUpdateCallback mCallback;
    void                                      *mCallbackContext;
};

DeclareTmfHandler(Client, kUriExtDiagnosticServerUpdate);
#endif // OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_ENABLE

} // namespace ExtNetworkDiagnostic

} // namespace ot

#endif // OT_CORE_THREAD_EXT_NETWORK_DIAGNOSTIC_HPP_
