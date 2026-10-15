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

/**
 * @file
 *   This file includes definitions for the Link Extension class.
 */

#ifndef OT_CORE_MAC_LINK_EXT_HPP_
#define OT_CORE_MAC_LINK_EXT_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_RADIO && OPENTHREAD_CONFIG_LINK_EXT_ENABLE

#include <openthread/link_ext.h>

#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "mac/channel_mask.hpp"
#include "mac/mac_frame.hpp"
#include "mac/sub_mac.hpp"
#include "radio/radio.hpp"

#if !OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
#error "OPENTHREAD_CONFIG_LINK_EXT_ENABLE requires OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE"
#endif

#if !OT_CONFIG_MAC_TARGET_TIME_TX_ENABLE
#error "OPENTHREAD_CONFIG_LINK_EXT_ENABLE requires target time TX support (e.g. CSL transmitter or software TX timing)"
#endif

#if (OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES > 0) && \
    (OPENTHREAD_CONFIG_LINK_EXT_MAC_NEIGHBOR_TABLE_MAX_ENTRIES == 0)
#error "OPENTHREAD_CONFIG_LINK_EXT_MAC_NEIGHBOR_TABLE_MAX_ENTRIES must be non-zero when the key table is enabled"
#endif

namespace ot {
namespace Mac {

class LinkRaw;

/**
 * Implements link-layer extensions built on top of the raw link-layer (`LinkRaw`), such as burst transmission and
 * periodic receive schedules.
 */
class LinkExt : public InstanceLocator, private NonCopyable
{
    friend class LinkRaw;

public:
    /**
     * Initializes the object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     */
    explicit LinkExt(Instance &aInstance);

    /**
     * Starts a burst transmission of the transmit frame of the raw link-layer.
     *
     * See `otLinkExtTransmitBurst()` for details.
     *
     * @param[in]  aConfig    The burst configuration.
     * @param[in]  aCallback  A pointer to a function called when the burst ends.
     *
     * @retval kErrorNone          Successfully started the burst.
     * @retval kErrorInvalidArgs   The configuration or the transmit frame is invalid for a burst.
     * @retval kErrorInvalidState  The raw link-layer isn't enabled.
     * @retval kErrorBusy          A burst transmission is ongoing.
     */
    Error TransmitBurst(const otLinkExtBurstConfig &aConfig, otLinkExtBurstDone aCallback);

    /**
     * Stops the ongoing burst transmission.
     *
     * See `otLinkExtStopBurst()` for details.
     *
     * @retval kErrorNone          The burst is stopped or will stop when the frame being transmitted is done, or no
     *                             burst is ongoing.
     * @retval kErrorInvalidState  The raw link-layer isn't enabled.
     */
    Error StopBurst(void);

    /**
     * Gets the configuration of the ongoing burst transmission.
     *
     * See `otLinkExtGetBurstConfig()` for details.
     *
     * @param[out] aConfig  A reference to return the configuration.
     */
    void GetBurstConfig(otLinkExtBurstConfig &aConfig) const;

    /**
     * Indicates whether a burst transmission is ongoing.
     *
     * @retval TRUE   A burst transmission is ongoing.
     * @retval FALSE  No burst transmission is ongoing.
     */
    bool IsBurstActive(void) const { return mBurstCount > 0; }

    /**
     * Records the radio state requested by the raw link-layer for the time between burst ticks.
     *
     * Called on a raw link-layer receive or sleep request.
     *
     * @param[in]  aIsReceiving  TRUE for a receive request, FALSE for a sleep request.
     *
     * @retval TRUE   A burst frame is with `SubMac`, which would drop it, so the request is applied once the tick is
     *                done instead.
     * @retval FALSE  The request can be applied right away.
     */
    bool ShouldDeferIdleState(bool aIsReceiving);

#if (OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES > 0)
    static constexpr uint8_t kMaxPeriodicRxSchedules =
        OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES; ///< Max number of periodic RX schedules.

    /**
     * Represents a periodic receive schedule (see `otLinkExtPeriodicRxSchedule`).
     */
    typedef otLinkExtPeriodicRxSchedule PeriodicRxSchedule;

    /**
     * Atomically replaces the periodic RX schedules.
     *
     * Array order defines priority (index 0 has highest priority). Overlapping windows are arbitrated as described
     * in "Periodic RX arbitration" below. Passing zero schedules disables periodic RX.
     *
     * @param[in] aSchedules     A pointer to an array of schedules (can be `nullptr` if @p aNumSchedules is zero).
     * @param[in] aNumSchedules  The number of schedules in @p aSchedules.
     *
     * See `otLinkExtSetPeriodicRxSchedules()` for details.
     *
     * @retval kErrorNone         Successfully updated the schedules.
     * @retval kErrorNoBufs       More than `kMaxPeriodicRxSchedules` schedules. The previous schedules are unchanged.
     * @retval kErrorInvalidArgs  A schedule is invalid. The previous schedules are left unchanged.
     */
    Error SetPeriodicRxSchedules(const PeriodicRxSchedule *aSchedules, uint8_t aNumSchedules);

    /**
     * Gets the current periodic RX schedules.
     *
     * @param[out] aNumSchedules  A reference to output the number of schedules.
     *
     * @returns A pointer to the array of current schedules.
     */
    const PeriodicRxSchedule *GetPeriodicRxSchedules(uint8_t &aNumSchedules) const
    {
        aNumSchedules = mNumPeriodicRxSchedules;
        return mPeriodicRxSchedules;
    }

    /**
     * Determines the periodic RX window to schedule now.
     *
     * Called by `SubMac::ListenScheduler` whenever it evaluates its listen sources. The returned window is passed
     * to `SubMac::ReceiveAt()`, and the scheduler evaluates again at the window start time.
     *
     * @param[in]  aNow      The current radio time.
     * @param[out] aTimedRx  A reference to output the window.
     *
     * @retval TRUE   A window is provided in @p aTimedRx.
     * @retval FALSE  There is no window to schedule.
     */
    bool DeterminePeriodicRxWindow(const Radio::SyncedTime &aNow, SubMac::TimedRx &aTimedRx);
#endif

    /**
     * Sets the MAC frame filter policy.
     *
     * See `otLinkExtSetMacFilterPolicy()` for details.
     *
     * @param[in] aPolicy  The policy bitmask (`OT_LINK_EXT_MAC_FILTER_POLICY_*` flags).
     *
     * @retval kErrorNone            Successfully set the policy.
     * @retval kErrorInvalidArgs     @p aPolicy contains an unknown flag.
     * @retval kErrorNotImplemented  @p aPolicy requires the MAC key table, which is disabled.
     */
    Error SetMacFilterPolicy(uint8_t aPolicy);

    /**
     * Gets the MAC frame filter policy.
     *
     * @returns The policy bitmask (`OT_LINK_EXT_MAC_FILTER_POLICY_*` flags).
     */
    uint8_t GetMacFilterPolicy(void) const { return mMacFilterPolicy; }

#if (OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES > 0)
    static constexpr uint8_t kMaxMacKeyTableEntries      = OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES;
    static constexpr uint8_t kMaxMacNeighborTableEntries = OPENTHREAD_CONFIG_LINK_EXT_MAC_NEIGHBOR_TABLE_MAX_ENTRIES;

    /**
     * Represents an entry of the MAC key table (see `otLinkExtMacKeyTableEntry`).
     */
    class MacKeyTableEntry : public otLinkExtMacKeyTableEntry
    {
    public:
        // Key ID lengths: Mode 0 has no Key ID, Mode 1 has a Key Index, and
        // Mode 2 has a Key Source followed by a Key Index.
        static constexpr uint8_t kKeyIndexSize     = sizeof(uint8_t);
        static constexpr uint8_t kKeyIdLengthMode0 = Frame::kKeySourceSizeMode0;
        static constexpr uint8_t kKeyIdLengthMode1 = Frame::kKeySourceSizeMode1 + kKeyIndexSize;
        static constexpr uint8_t kKeyIdLengthMode2 = Frame::kKeySourceSizeMode2 + kKeyIndexSize;

        /**
         * Indicates whether the entry is valid.
         *
         * @retval TRUE   The entry is valid.
         * @retval FALSE  The entry is invalid.
         */
        bool IsValid(void) const;

        /**
         * Indicates whether a secured frame matches the entry.
         *
         * @param[in] aParseInfo  The parse info of the frame.
         *
         * @retval TRUE   The frame matches the entry.
         * @retval FALSE  The frame does not match the entry.
         */
        bool Matches(const RxFrame::ParseInfo &aParseInfo) const;

        /**
         * Verifies the MIC of a secured frame using the key of the entry.
         *
         * The MIC is verified on a copy of the frame, which is left unmodified.
         *
         * @param[in] aParseInfo     The parse info of the frame.
         * @param[in] aNonceExtAddr  The extended address used for the AES-CCM* nonce.
         *
         * @retval TRUE   The MIC is verified.
         * @retval FALSE  The MIC verification failed.
         */
        bool VerifyMic(const RxFrame::ParseInfo &aParseInfo, const ExtAddress &aNonceExtAddr) const;

    private:
        Error GetKeyIdMode(uint8_t &aKeyIdMode) const;
    };

    /**
     * Represents an entry of the MAC neighbor table (see `otLinkExtMacNeighborTableEntry`).
     */
    class MacNeighborTableEntry : public otLinkExtMacNeighborTableEntry
    {
    public:
        /**
         * Gets the extended address.
         *
         * @returns The extended address.
         */
        const ExtAddress &GetExtAddress(void) const { return AsCoreType(&mExtAddress); }

        /**
         * Indicates whether the PAN ID is set.
         *
         * @retval TRUE   The PAN ID is set.
         * @retval FALSE  The PAN ID is not set.
         */
        bool HasPanId(void) const { return mPanId != kPanIdBroadcast; }

        /**
         * Indicates whether the short address is set.
         *
         * @retval TRUE   The short address is set.
         * @retval FALSE  The short address is not set.
         */
        bool HasShortAddress(void) const { return mShortAddress != kShortAddrBroadcast; }

        /**
         * Indicates whether the entry identifies the same neighbor as another entry.
         *
         * @param[in] aOther  The other entry.
         *
         * @retval TRUE   The extended address and key table index are equal.
         * @retval FALSE  The extended address or key table index differs.
         */
        bool IsSameNeighbor(const otLinkExtMacNeighborTableEntry &aOther) const;

        /**
         * Indicates whether a secured frame matching a key table entry matches the neighbor.
         *
         * @param[in] aParseInfo      The parse info of the frame.
         * @param[in] aKeyTableIndex  The index of the key table entry which the frame matches.
         *
         * @retval TRUE   The frame matches the neighbor.
         * @retval FALSE  The frame does not match the neighbor.
         */
        bool Matches(const RxFrame::ParseInfo &aParseInfo, uint8_t aKeyTableIndex) const;
    };

    /**
     * Atomically replaces the MAC key table.
     *
     * See `otLinkExtSetMacKeyTable()` for details.
     *
     * @param[in] aEntries     A pointer to an array of entries (can be `nullptr` if @p aNumEntries is zero).
     * @param[in] aNumEntries  The number of entries in @p aEntries.
     *
     * @retval kErrorNone         Successfully set the table.
     * @retval kErrorNoBufs       More than `kMaxMacKeyTableEntries` entries. The table is left unchanged.
     * @retval kErrorInvalidArgs  An entry is invalid. The table is left unchanged.
     */
    Error SetMacKeyTable(const otLinkExtMacKeyTableEntry *aEntries, uint8_t aNumEntries);

    /**
     * Atomically replaces the MAC neighbor table.
     *
     * See `otLinkExtSetMacNeighborTable()` for details.
     *
     * @param[in] aEntries     A pointer to an array of entries (can be `nullptr` if @p aNumEntries is zero).
     * @param[in] aNumEntries  The number of entries in @p aEntries.
     *
     * @retval kErrorNone         Successfully set the table.
     * @retval kErrorNoBufs       More than `kMaxMacNeighborTableEntries` entries. The table is left unchanged.
     * @retval kErrorInvalidArgs  An entry is invalid, or two entries have the same extended address and key table
     *                            index. The table is left unchanged.
     */
    Error SetMacNeighborTable(const otLinkExtMacNeighborTableEntry *aEntries, uint8_t aNumEntries);

    /**
     * Gets the MAC neighbor table.
     *
     * @param[out] aNumEntries  A reference to output the number of entries.
     *
     * @returns A pointer to the array of entries.
     */
    const MacNeighborTableEntry *GetMacNeighborTable(uint8_t &aNumEntries) const
    {
        aNumEntries = mNumMacNeighborTableEntries;
        return mMacNeighborTable;
    }

    /**
     * Gets the MAC key table.
     *
     * @param[out] aNumEntries  A reference to output the number of entries.
     *
     * @returns A pointer to the array of entries.
     */
    const MacKeyTableEntry *GetMacKeyTable(uint8_t &aNumEntries) const
    {
        aNumEntries = mNumMacKeyTableEntries;
        return mMacKeyTable;
    }
#endif

private:
    static constexpr uint32_t kBurstTxAheadUs = OPENTHREAD_CONFIG_LINK_EXT_BURST_TX_AHEAD_US;

    // Called by `LinkRaw`.
    void Reset(void);
    void HandleLinkRawEnableChange(void);
    void HandleTransmitDone(TxFrame &aFrame);
    bool FilterReceivedFrame(RxFrame *aFrame, Error aError);

    static constexpr uint8_t kMacFilterPolicyKeyTable = OT_LINK_EXT_MAC_FILTER_POLICY_DROP_UNKNOWN_KEY |
                                                        OT_LINK_EXT_MAC_FILTER_POLICY_DROP_UNKNOWN_NEIGHBOR |
                                                        OT_LINK_EXT_MAC_FILTER_POLICY_DROP_REPLAYED;
    static constexpr uint8_t kMacFilterPolicyAll = OT_LINK_EXT_MAC_FILTER_POLICY_DROP_MALFORMED |
                                                   OT_LINK_EXT_MAC_FILTER_POLICY_DROP_UNSECURED |
                                                   kMacFilterPolicyKeyTable;

#if (OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES > 0)
    enum MacKeyTableResult : uint8_t
    {
        kMacKeyTableNoMatch,         // The key is unknown.
        kMacKeyTableAccepted,        // The frame is accepted.
        kMacKeyTableUnknownNeighbor, // The sender is not in the neighbor table.
        kMacKeyTableMicFailed,       // The MIC verification failed.
        kMacKeyTableReplayed,        // The frame counter is lower than the neighbor's next expected one.
    };

    bool              IsMode1MacKeyIndex(uint8_t aKeyIndex) const;
    MacKeyTableResult CheckMacKeyTable(const RxFrame::ParseInfo &aParseInfo);
#endif

    void  CancelBurst(void);
    void  FinishBurst(void);
    void  ApplyIdleState(void);
    Error StartBurst(const otLinkExtBurstConfig &aConfig);
    void  SkipMissedBurstSlots(Radio::Time32 aNow);
    void  StartBurstTimer(Radio::Time32 aNow);
    void  ScheduleNextBurstTick(void);
    void  HandleBurstTimer(void);
    void  SendBurstFrame(void);

#if (OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES > 0)
    /*
     * Periodic RX arbitration
     *
     * The periodic RX schedules are arbitrated here, and the resulting windows are passed one at a time to
     * `SubMac::ListenScheduler` as a listen source (see `DeterminePeriodicRxWindow()`). Against other listen sources
     * of `SubMac` (e.g. CSL receiver, which is not expected to be enabled on an RCP), the listen scheduler's own rule
     * applies (the earliest window that has not ended is scheduled).
     *
     * Rules among the periodic RX schedules (array order defines priority, index 0 has highest priority):
     *
     * 1. Owner: At any instant, the radio is owned by the highest-priority schedule whose window covers that instant.
     *    If no window covers the instant, there is no periodic RX window.
     *
     * 2. Truncation, not skipping: A lower-priority window overlapping a higher-priority window on a different
     *    channel is not dropped as a whole. It keeps the parts not covered by the higher-priority window, which can
     *    be a part before and/or a part after it.
     *
     * 3. Merging: Windows are formed from the owner channel, not the owner. A maximal interval during which the owner
     *    channel stays the same is one window, so overlapping or back-to-back windows on the same channel are merged
     *    regardless of their priorities. A merged window is capped at the largest schedule period. To bound the
     *    computation, a window is also ended after evaluating `kPeriodicRxMaxMergeEvents` events, so a longer
     *    continuous interval on the same channel is split into back-to-back windows.
     *
     * 4. Channel switch guard: When a higher-priority window preempts an ongoing lower-priority window on a different
     *    channel, the higher-priority schedule also owns the `kPeriodicRxGuard` interval right before its window
     *    start, so that the radio is listening on the new channel no later than the window start. The guard time is
     *    taken from the lower-priority window. No guard is added when the higher-priority window starts while no
     *    other window is active, or when the radio returns to a lower-priority window after a higher-priority window
     *    ends.
     *
     * 5. Minimum fragment: A truncated window (shorter than the full window duration of its owner) which is shorter
     *    than `kPeriodicRxMinFragment` (guard time plus the SHR duration, i.e. the time needed to detect a frame) is
     *    dropped.
     *
     * Example: schedule A (higher priority, channel 11) window [5000, 6600), schedule B (lower priority, channel 12)
     * window [4000, 8000), guard `G`:
     *
     *          4000          5000-G 5000          6600          8000
     *   A:                          |=============|
     *   B:       |========================================================|
     *   Result:  |== ch 12 ===|== ch 11 ==========|== ch 12 =====|
     *
     * Scheduling: since a subsequent `ReceiveAt()` replaces a window which has not started yet, the window following
     * the active one is provided only once the active one has started. If the active window changes (e.g. it is
     * truncated after the schedules are updated), it is provided again keeping its start time.
     *
     * 6. Frame in progress: The rules above define when windows start and end. If the radio has started receiving a
     *    frame (e.g. SHR detected) in a window, it completes the frame reception and any ACK transmission before
     *    switching to the channel of the next window or returning to Sleep, even if the frame extends into a
     *    higher-priority window. This is required of the radio platform by `otPlatRadioReceiveAt()`, so it needs no
     *    handling here. A frame failing the address filter is expected to be aborted as soon as the mismatch is
     *    determined, so that it does not delay the next window.
     *
     * Timed RX windows only take effect while the radio is in Sleep state. Other operations (receive, transmit,
     * energy scan) take precedence, see `otPlatRadioReceiveAt()`.
     *
     * Limitation: when the schedules are updated, a window already scheduled on the radio is not cancelled, since
     * `SubMac::ListenScheduler` does not cancel a window scheduled on the radio. Hence the previously scheduled
     * window may still occur once after the schedules are cleared or changed.
     */

    static constexpr uint32_t kPeriodicRxGuard       = OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_CHANNEL_SWITCH_GUARD_US;
    static constexpr uint32_t kPeriodicRxMinFragment = kPeriodicRxGuard + Radio::kHeaderShrDuration;
    static constexpr uint16_t kPeriodicRxMaxMergeEvents = 64;
    static constexpr uint8_t  kPeriodicRxNoOwner        = 0xff;

    bool                 IsValid(const PeriodicRxSchedule &aSchedule);
    static Radio::Time64 GetNextStart(const PeriodicRxSchedule &aSchedule, Radio::Time64 aTime);
    static bool          IsCovering(const PeriodicRxSchedule &aSchedule, Radio::Time64 aTime, Radio::Time64 &aEnd);
    static bool          IsInGuard(const PeriodicRxSchedule &aSchedule, Radio::Time64 aTime);
    static Radio::Time64 GetNextEvent(const PeriodicRxSchedule &aSchedule, Radio::Time64 aTime);

    uint8_t       GetPeriodicRxOwner(Radio::Time64 aTime) const;
    Radio::Time64 GetNextPeriodicRxEvent(Radio::Time64 aTime) const;
    bool          FindNextPeriodicRxWindow(Radio::Time64 aFrom, SubMac::TimedRx &aWindow) const;
#endif

    using BurstTimer = TimerMicroIn<LinkExt, &LinkExt::HandleBurstTimer>;

    otLinkExtBurstDone mBurstDoneCallback;
    BurstTimer         mBurstTimer;
    ChannelMask        mBurstChannelMask;
    Radio::Time32      mBurstTxTime; // Slot start time (radio clock) of the current tick.
    uint32_t           mBurstPeriodUs;
    uint16_t           mBurstCount; // Remaining ticks, including the current one.
    bool               mIsTimedBurst;
    bool               mIsBurstTxPending;     // A burst frame is with `SubMac`.
    bool               mIsBurstStopRequested; // Stop is requested while a burst frame is with the radio.
    bool               mIsReceiving;          // Idle state between burst ticks: receive (true) or sleep (false).

#if (OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES > 0)
    uint8_t            mNumPeriodicRxSchedules;
    PeriodicRxSchedule mPeriodicRxSchedules[kMaxPeriodicRxSchedules];
    SubMac::TimedRx    mPeriodicRxWindow; // The last window provided to `SubMac::ListenScheduler`.
#endif

    uint8_t mMacFilterPolicy;
#if (OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES > 0)
    uint8_t               mNumMacKeyTableEntries;
    uint8_t               mNumMacNeighborTableEntries;
    MacKeyTableEntry      mMacKeyTable[kMaxMacKeyTableEntries];
    MacNeighborTableEntry mMacNeighborTable[kMaxMacNeighborTableEntries];
#endif
};

} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_RADIO && OPENTHREAD_CONFIG_LINK_EXT_ENABLE

#endif // OT_CORE_MAC_LINK_EXT_HPP_
