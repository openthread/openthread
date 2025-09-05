/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file includes definitions for forwarding IPv6 datagrams across the Thread mesh.
 */

#ifndef MESH_FORWARDER_HPP_
#define MESH_FORWARDER_HPP_

#include "openthread-core-config.h"

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/frame_data.hpp"
#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/non_copyable.hpp"
#include "common/owned_ptr.hpp"
#include "common/tasklet.hpp"
#include "common/time_ticker.hpp"
#include "mac/channel_mask.hpp"
#include "mac/data_poll_sender.hpp"
#include "mac/mac.hpp"
#include "mac/mac_frame.hpp"
#include "net/ip6.hpp"
#include "thread/address_resolver.hpp"
#include "thread/child.hpp"
#include "thread/indirect_sender.hpp"
#include "thread/lowpan.hpp"
#include "thread/network_data_leader.hpp"
#include "thread/thread_link_info.hpp"

namespace ot {

namespace Mle {
class DiscoverScanner;
}

namespace Utils {
class HistoryTracker;
}

/**
 * @addtogroup core-mesh-forwarding
 *
 * @brief
 *   This module includes definitions for mesh forwarding within Thread.
 *
 * @{
 */

/**
 * Implements mesh forwarding within Thread.
 */
class MeshForwarder : public InstanceLocator, private NonCopyable
{
    friend class Mac::Mac;
    friend class Instance;
    friend class DataPollSender;
    friend class IndirectSender;
    friend class Ip6::Ip6;
    friend class Mle::DiscoverScanner;
    friend class TimeTicker;

public:
    /**
     * Represents the IPv6 message counters, tracking the number IPv6 message TX and RX that succeeded or failed.
     */
    class Counters : public otIpCounters, public Clearable<Counters>
    {
        friend class MeshForwarder;
        friend class IndirectSender;

    private:
        Counters(void) { Clear(); }

        void UpdateOnTxDone(const Message &aMessage, bool aTxSuccess);
        void UpdateOnRx(const Message &aMessage);
        void UpdateOnDrop(const Message &aMessage);
    };

    /**
     * Initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit MeshForwarder(Instance &aInstance);

    /**
     * Enables mesh forwarding and the IEEE 802.15.4 MAC layer.
     */
    void Start(void);

    /**
     * Disables mesh forwarding and the IEEE 802.15.4 MAC layer.
     */
    void Stop(void);

    /**
     * Submits a message to the mesh forwarder for forwarding.
     *
     * @param[in]  aMessagePtr  An owned pointer to a message (transfer ownership).
     */
    void SendMessage(OwnedPtr<Message> aMessagePtr);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    /**
     * Sends an empty data frame to the parent.
     *
     * @retval kErrorNone          Successfully enqueued an empty message.
     * @retval kErrorInvalidState  Device is not in Rx-Off-When-Idle mode or it has no parent.
     * @retval kErrorNoBufs        Insufficient message buffers available.
     */
    Error SendEmptyMessage(void);
#endif

    /**
     * Is called by the address resolver when an EID-to-RLOC mapping has been resolved.
     *
     * @param[in]  aEid    A reference to the EID that has been resolved.
     * @param[in]  aError  kErrorNone on success and kErrorDrop otherwise.
     */
    void HandleResolved(const Ip6::Address &aEid, Error aError);

    /**
     * Indicates whether or not rx-on-when-idle mode is enabled.
     *
     * @retval TRUE   The rx-on-when-idle mode is enabled.
     * @retval FALSE  The rx-on-when-idle-mode is disabled.
     */
    bool GetRxOnWhenIdle(void) const;

    /**
     * Sets the rx-on-when-idle mode
     *
     * @param[in]  aRxOnWhenIdle  TRUE to enable, FALSE otherwise.
     */
    void SetRxOnWhenIdle(bool aRxOnWhenIdle);

#if OPENTHREAD_FTD
    typedef IndirectSender::MessageChecker MessageChecker; ///< General predicate function checking a message.

    /**
     * Removes and frees messages queued for a child, based on a given predicate.
     *
     * The `aChild` can be either sleepy or non-sleepy.
     *
     * @param[in] aChild            The child whose messages are to be evaluated.
     * @param[in] aMessageChecker   The predicate function to filter messages.
     */
    void RemoveMessagesForChild(Child &aChild, MessageChecker aMessageChecker);
#endif

    /**
     * Frees unicast/multicast MLE Data Responses from Send Message Queue if any.
     */
    void RemoveDataResponseMessages(void);

    /**
     * Evicts the message with lowest priority in the send queue.
     *
     * @param[in]  aPriority  The highest priority level of the evicted message.
     *
     * @retval kErrorNone       Successfully evicted a low priority message.
     * @retval kErrorNotFound   No low priority messages available to evict.
     */
    Error EvictMessage(Message::Priority aPriority);

    /**
     * Retrieves information about the send queue and the reassembly queue.
     *
     * Provides details such as the number of messages and data buffers currently utilized by the priority send queue
     * and the message reassembly queue.
     *
     * @param[out] aSendQueueInfo         A `PriorityQueue::Info` to populate with info about the send queue.
     * @param[out] aReassemblyQueueInfo   A `MessageQueue::Info` to populate with info about the reassembly queue.
     */
    void GetQueueInfo(PriorityQueue::Info &aSendQueueInfo, MessageQueue::Info &aReassemblyQueueInfo) const
    {
        mSendQueue.GetInfo(aSendQueueInfo), mReassemblyList.GetInfo(aReassemblyQueueInfo);
    }

    /**
     * Returns a reference to the IP level counters.
     *
     * @returns A reference to the IP level counters.
     */
    const Counters &GetCounters(void) const { return mCounters; }

    /**
     * Resets the IP level counters.
     */
    void ResetCounters(void) { mCounters.Clear(); }

#if OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_ENABLE
    /**
     * Gets the time-in-queue histogram for messages in the TX queue.
     *
     * Histogram of the time-in-queue of messages in the transmit queue is collected. The time-in-queue is tracked for
     * direct transmissions only and is measured as the duration from when a message is added to the transmit queue
     * until it is passed to the MAC layer for transmission or dropped.
     *
     * The histogram is returned as an array of `uint32_t` values with `aNumBins` entries. The first entry in the array
     * (at index 0) represents the number of messages with a time-in-queue less than `aBinInterval`. The second entry
     * represents the number of messages with a time-in-queue greater than or equal to `aBinInterval`, but less than
     * `2 * aBinInterval`. And so on. The last entry represents the number of messages with time-in-queue  greater than
     * or * equal to `(aNumBins - 1) * aBinInterval`.
     *
     * The collected statistics can be reset by calling `ResetTimeInQueueStat()`. The histogram information is
     * collected since the OpenThread instance was initialized or since the last time statistics collection was reset
     * by calling the `ResetTimeInQueueStat()`.
     *
     * @param[out] aNumBins       Reference to return the number of bins in histogram (array length).
     * @param[out] aBinInterval   Reference to return the histogram bin interval length in milliseconds.
     *
     * @returns A pointer to an array of @p aNumBins entries representing the collected histogram info.
     */
    const uint32_t *GetTimeInQueueHistogram(uint16_t &aNumBins, uint32_t &aBinInterval) const
    {
        return mTxQueueStats.GetHistogram(aNumBins, aBinInterval);
    }

    /**
     * Gets the maximum time-in-queue for messages in the TX queue.
     *
     * The time-in-queue is tracked for direct transmissions only and is measured as the duration from when a message
     * is added to the transmit queue until it is passed to the MAC layer for transmission or dropped.
     *
     * The collected statistics can be reset by calling `ResetTimeInQueueStat()`.
     *
     * @returns The maximum time-in-queue in milliseconds for all messages in the TX queue (so far).
     */
    uint32_t GetMaxTimeInQueue(void) const { return mTxQueueStats.GetMaxInterval(); }

    /**
     * Resets the TX queue time-in-queue statistics.
     */
    void ResetTimeInQueueStat(void) { mTxQueueStats.Clear(); }
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    /**
     * Handles a deferred ack.
     *
     * Some radio links can use deferred ack logic, where a tx request always report `HandleSentFrame()` quickly. The
     * link layer would wait for the ack and report it at a later time using this method.
     *
     * The link layer is expected to call `HandleDeferredAck()` (with success or failure status) for every tx request
     * on the radio link.
     *
     * @param[in] aNeighbor  The neighbor for which the deferred ack status is being reported.
     * @param[in] aError     The deferred ack error status: `kErrorNone` to indicate a deferred ack was received,
     *                       `kErrorNoAck` to indicate an ack timeout.
     */
    void HandleDeferredAck(Neighbor &aNeighbor, Error aError);
#endif

private:
    static constexpr uint8_t kFailedRouterTransmissions      = 4;
    static constexpr uint8_t kFailedCslDataPollTransmissions = 15;

    static constexpr uint8_t kReassemblyTimeout      = OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT; // in seconds.
    static constexpr uint8_t kMeshHeaderFrameMtu     = OT_RADIO_FRAME_MAX_SIZE; // Max MTU with a Mesh Header frame.
    static constexpr uint8_t kMeshHeaderFrameFcsSize = sizeof(uint16_t);        // Frame FCS size for Mesh Header frame.

    // Hops left to use in lowpan mesh header: We use `kMaxRouteCost` as
    // max hops between routers within Thread  mesh. We then add two
    // for possibility of source or destination being a child
    // (requiring one hop) and one as additional guard increment.
    static constexpr uint8_t kMeshHeaderHopsLeft = Mle::kMaxRouteCost + 3;

    static constexpr uint32_t kTxDelayInterval = OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_INTERVAL; // In msec

#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
    static constexpr uint32_t kTimeInQueueMarkEcn = OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_MARK_ECN_INTERVAL;
    static constexpr uint32_t kTimeInQueueDropMsg = OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_DROP_MSG_INTERVAL;
#endif

    enum MessageAction : uint8_t
    {
        kMessageReceive,         // Indicates that the message was received.
        kMessageTransmit,        // Indicates that the message was sent.
        kMessagePrepareIndirect, // Indicates that the message is being prepared for indirect tx.
        kMessageDrop,            // Indicates that the outbound message is dropped (e.g., dst unknown).
        kMessageReassemblyDrop,  // Indicates that the message is being dropped from reassembly list.
        kMessageEvict,           // Indicates that the message was evicted.
#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
        kMessageMarkEcn,       // Indicates that ECN is marked on an outbound message by delay-aware queue management.
        kMessageQueueMgmtDrop, // Indicates that an outbound message is dropped by delay-aware queue management.
#endif
#if (OPENTHREAD_CONFIG_MAX_FRAMES_IN_DIRECT_TX_QUEUE > 0)
        kMessageFullQueueDrop, // Indicates message drop due to reaching max allowed frames in direct tx queue.
#endif
    };

    struct RxInfo : public InstanceLocator
    {
        static constexpr uint16_t kInfoStringSize = 70;

        typedef String<kInfoStringSize> InfoString;

        explicit RxInfo(Instance &aInstance)
            : InstanceLocator(aInstance)
            , mParsedIp6Headers(false)
        {
        }

        const Mac::Address &GetSrcAddr(void) const { return mMacAddrs.mSource; }
        const Mac::Address &GetDstAddr(void) const { return mMacAddrs.mDestination; }
        bool                IsLinkSecurityEnabled(void) const { return mLinkInfo.IsLinkSecurityEnabled(); }
        Error               ParseIp6Headers(void);
        InfoString          ToString(void) const;

        FrameData      mFrameData;
        ThreadLinkInfo mLinkInfo;
        Mac::Addresses mMacAddrs;
        Ip6::Headers   mIp6Headers;
        bool           mParsedIp6Headers;
    };

#if OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
    static constexpr uint16_t kQmFwdEntries = OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_FRAG_TAG_ENTRY_LIST_SIZE;
#else
    static constexpr uint16_t kQmFwdEntries = 0;
#endif
    static constexpr uint16_t kPrioFwdEntries = OPENTHREAD_CONFIG_NUM_FRAGMENT_PRIORITY_ENTRIES;
    static constexpr uint16_t kFwdInfoEntries = OT_MAX(kPrioFwdEntries, kQmFwdEntries);

    class FwdFrameInfo
    {
        // Tracks information (priority, drop status) for forwarded
        // mesh-header fragmented frames. This ensures consistent
        // priority assignment to all fragments of the same message and
        // facilitates delay-aware queue management, where dropping
        // one fragment leads to dropping all subsequent fragments of
        // the same message.

    public:
        enum ExpireChecker : uint8_t
        {
            kIsExpired,
        };

        struct Info
        {
            uint16_t mSrcRloc16;
            uint16_t mDatagramTag;
        };

        void Init(uint16_t aSrcRloc16, uint16_t aDatagramTag, Message::Priority aPriority);
        bool Matches(const Info &aInfo) const;
        bool Matches(const ExpireChecker) const { return IsExpired(); }
        void ResetLifetime(void) { mLifetime = kLifetime; }
        void DecrementLifetime(void) { mLifetime--; }
        bool IsExpired(void) const { return (mLifetime == 0); }
#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
        bool ShouldDrop(void) const { return mShouldDrop; }
        void MarkToDrop(void) { mShouldDrop = true; }
#endif
        Message::Priority GetPriority(void) const { return static_cast<Message::Priority>(mPriority); }

    private:
#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
        static constexpr uint8_t kRetainTime = OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_FRAG_TAG_RETAIN_TIME;
#else
        static constexpr uint8_t kRetainTime = 0;
#endif
        static constexpr uint8_t kLifetime = OT_MAX(kReassemblyTimeout, kRetainTime);

        uint16_t mSrcRloc16;
        uint16_t mDatagramTag;
        uint8_t  mLifetime;
        uint8_t  mPriority : 2;
#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
        bool mShouldDrop : 1;
#endif

        static_assert(Message::kNumPriorities <= 4, "mPriority as a 2-bit does not fit all `Priority` values");
    };

    using FwdFrameInfoArray = Array<FwdFrameInfo, kFwdInfoEntries>;

#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_ENABLE
    class TxQueueStats : public Clearable<TxQueueStats>
    {
    public:
        const uint32_t *GetHistogram(uint16_t &aNumBins, uint32_t &aBinInterval) const;
        uint32_t        GetMaxInterval(void) const { return mMaxInterval; }
        void            UpdateFor(const Message &aMessage);

    private:
        static constexpr uint32_t kHistMaxInterval = OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_HISTOGRAM_MAX_INTERVAL;
        static constexpr uint32_t kHistBinInterval = OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_HISTOGRAM_BIN_INTERVAL;
        static constexpr uint16_t kNumHistBins     = (kHistMaxInterval + kHistBinInterval - 1) / kHistBinInterval;

        uint32_t mMaxInterval;
        uint32_t mHistogram[kNumHistBins];
    };
#endif

#if OPENTHREAD_FTD
    bool  IsReachable(uint16_t aMeshDest, const Ip6::Header &aIp6Header) const;
    void  CheckReachabilityToSendIcmpError(const Message &aMessage, const Mac::Addresses &aMacAddrs);
    Error CheckReachabilityToSendIcmpError(RxInfo &aRxInfo);
#endif
    void     UpdateEidRlocCacheAndStaleChild(RxInfo &aRxInfo);
    Error    FrameToMessage(RxInfo &aRxInfo, uint16_t aDatagramSize, Message *&aMessage);
    Message *PrepareNextDirectTransmission(void);
    void     HandleMesh(RxInfo &aRxInfo);
    void     ResolveRoutingLoops(uint16_t aSourceRloc16, uint16_t aDestRloc16);
    void     HandleFragment(RxInfo &aRxInfo);
    void     HandleLowpanHc(RxInfo &aRxInfo);

#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
    Error UpdateEcnOrDrop(Message &aMessage, bool aPreparingToSend);
    Error RemoveAgedMessages(void);
#endif
#if (OPENTHREAD_CONFIG_MAX_FRAMES_IN_DIRECT_TX_QUEUE > 0)
    bool IsDirectTxQueueOverMaxFrameThreshold(void) const;
    void ApplyDirectTxQueueLimit(Message &aMessage);
#endif
    void  SendDestinationUnreachable(uint16_t aMeshSource, const Ip6::Headers &aIp6Headers);
    Error UpdateIp6Route(Message &aMessage);
    Error UpdateIp6RouteFtd(const Ip6::Header &aIp6Header, Message &aMessage);
    Error UpdateMeshRoute(Message &aMessage);
    bool  UpdateReassemblyList(void);
    void  UpdateFragmentPriority(Lowpan::FragmentHeader &aFragmentHeader,
                                 uint16_t                aFragmentLength,
                                 uint16_t                aSrcRloc16,
                                 Message::Priority       aPriority);
    Error HandleDatagram(Message &aMessage, const Mac::Address &aMacSource);
    void  ClearReassemblyList(void);
    void  HandleDiscoverComplete(void);

    void          HandleReceivedFrame(Mac::RxFrame &aFrame);
    Mac::TxFrame *HandleFrameRequest(Mac::TxFrames &aTxFrames);
    Neighbor     *UpdateNeighborOnSentFrame(Mac::TxFrame       &aFrame,
                                            Error               aError,
                                            const Mac::Address &aMacDest,
                                            bool                aIsDataPoll);
    void UpdateNeighborLinkFailures(Neighbor &aNeighbor, Error aError, bool aAllowNeighborRemove, uint8_t aFailLimit);
    void HandleSentFrame(Mac::TxFrame &aFrame, Error aError);
    void UpdateSendMessage(Error aFrameTxError, Mac::Address &aMacDest, Neighbor *aNeighbor);
    void FinalizeMessageDirectTx(Message &aMessage, Error aError);
    void FinalizeAndRemoveMessage(Message &aMessage, Error aError, MessageAction aAction);
    bool RemoveMessageIfNoPendingTx(Message &aMessage);

    void HandleTimeTick(void);
    void ScheduleTransmissionTask(void);

    Error GetFramePriority(RxInfo &aRxInfo, Message::Priority &aPriority);

#if OPENTHREAD_FTD
    void          FinalizeMessageIndirectTxs(Message &aMessage);
    FwdFrameInfo *FindFwdFrameInfoEntry(uint16_t aSrcRloc16, uint16_t aDatagramTag);
    bool          UpdateFwdFrameInfoArrayOnTimeTick(void);

    Error GetFragmentPriority(Lowpan::FragmentHeader &aFragmentHeader,
                              uint16_t                aSrcRloc16,
                              Message::Priority      &aPriority);
    void  GetForwardFramePriority(RxInfo &aRxInfo, Message::Priority &aPriority);

#endif

    void PauseMessageTransmissions(void) { mTxPaused = true; }
    void ResumeMessageTransmissions(void);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_ENABLE
    void HandleTxDelayTimer(void);
#endif

    void LogMessage(MessageAction aAction, const Message &aMessage);
    void LogMessage(MessageAction aAction, const Message &aMessage, Error aError);
    void LogMessage(MessageAction aAction, const Message &aMessage, Error aError, const Mac::Address *aAddress);
    void LogFrame(const char *aActionText, const Mac::Frame &aFrame, Error aError);
    void LogFragmentFrameDrop(Error aError, const RxInfo &aRxInfo, const Lowpan::FragmentHeader &aFragmentHeader);
    void LogLowpanHcFrameDrop(Error aError, const RxInfo &aRxInfo);

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE)
    const char *MessageActionToString(MessageAction aAction, Error aError);
    const char *MessagePriorityToString(const Message &aMessage);

#if OPENTHREAD_FTD
    Error LogMeshFragmentHeader(MessageAction       aAction,
                                const Message      &aMessage,
                                const Mac::Address *aMacAddress,
                                Error               aError,
                                uint16_t           &aOffset,
                                Mac::Addresses     &aMeshAddrs,
                                LogLevel            aLogLevel);
    void  LogMeshIpHeader(const Message        &aMessage,
                          uint16_t              aOffset,
                          const Mac::Addresses &aMeshAddrs,
                          LogLevel              aLogLevel);
    void  LogMeshMessage(MessageAction       aAction,
                         const Message      &aMessage,
                         const Mac::Address *aAddress,
                         Error               aError,
                         LogLevel            aLogLevel);
#endif
#if OPENTHREAD_CONFIG_LOG_SRC_DST_IP_ADDRESSES
    void LogIp6AddressAndPort(const char *aLabel, const Ip6::Address &aAddress, uint16_t aPort, LogLevel aLogLevel);
#endif
    void LogIp6SourceDestAddresses(const Ip6::Headers &aHeaders, LogLevel aLogLevel);
    void LogIp6Message(MessageAction       aAction,
                       const Message      &aMessage,
                       const Mac::Address *aAddress,
                       Error               aError,
                       LogLevel            aLogLevel);
    void AppendSecErrorPrioRssRadioLabelsToLogString(StringWriter  &aString,
                                                     MessageAction  aAction,
                                                     const Message &aMessage,
                                                     Error          aError);
    void AppendMacAddrToLogString(StringWriter &aString, MessageAction aAction, const Mac::Address *aMacAddress);
#endif // #if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE)

    using TxTask = TaskletIn<MeshForwarder, &MeshForwarder::ScheduleTransmissionTask>;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_ENABLE
    using TxDelayTimer = TimerMilliIn<MeshForwarder, &MeshForwarder::HandleTxDelayTimer>;
#endif

    PriorityQueue mSendQueue;
    MessageQueue  mReassemblyList;
    uint16_t      mMessageNextOffset;

    Message *mSendMessage;

    Mac::Addresses mMacAddrs;
    uint16_t       mMeshSource;
    uint16_t       mMeshDest;
    bool           mAddMeshHeader : 1;
    bool           mEnabled : 1;
    bool           mTxPaused : 1;
    bool           mSendBusy : 1;
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_ENABLE
    bool         mDelayNextTx : 1;
    TxDelayTimer mTxDelayTimer;
#endif

    TxTask mScheduleTransmissionTask;

    Counters mCounters;

#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    IndirectSender mIndirectSender;
#endif

#if OPENTHREAD_FTD
    FwdFrameInfoArray mFwdFrameInfoArray;
#endif

    DataPollSender mDataPollSender;

#if OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_ENABLE
    TxQueueStats mTxQueueStats;
#endif
};

/**
 * @}
 */

} // namespace ot

#endif // MESH_FORWARDER_HPP_
