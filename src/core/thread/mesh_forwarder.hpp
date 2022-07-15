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
#include "common/tasklet.hpp"
#include "common/time_ticker.hpp"
#include "mac/channel_mask.hpp"
#include "mac/data_poll_sender.hpp"
#include "mac/mac.hpp"
#include "mac/mac_frame.hpp"
#include "net/ip6.hpp"
#include "thread/address_resolver.hpp"
#include "thread/indirect_sender.hpp"
#include "thread/lowpan.hpp"
#include "thread/network_data_leader.hpp"
#include "thread/topology.hpp"

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
 * This class represents link-specific information for messages received from the Thread radio.
 *
 */
class ThreadLinkInfo : public otThreadLinkInfo, public Clearable<ThreadLinkInfo>
{
public:
    /**
     * This method returns the IEEE 802.15.4 Source PAN ID.
     *
     * @returns The IEEE 802.15.4 Source PAN ID.
     *
     */
    Mac::PanId GetPanId(void) const { return mPanId; }

    /**
     * This method returns the IEEE 802.15.4 Channel.
     *
     * @returns The IEEE 802.15.4 Channel.
     *
     */
    uint8_t GetChannel(void) const { return mChannel; }

    /**
     * This method returns whether the Destination PAN ID is broadcast.
     *
     * @retval TRUE   If Destination PAN ID is broadcast.
     * @retval FALSE  If Destination PAN ID is not broadcast.
     *
     */
    bool IsDstPanIdBroadcast(void) const { return mIsDstPanIdBroadcast; }

    /**
     * This method indicates whether or not link security is enabled.
     *
     * @retval TRUE   If link security is enabled.
     * @retval FALSE  If link security is not enabled.
     *
     */
    bool IsLinkSecurityEnabled(void) const { return mLinkSecurity; }

    /**
     * This method returns the Received Signal Strength (RSS) in dBm.
     *
     * @returns The Received Signal Strength (RSS) in dBm.
     *
     */
    int8_t GetRss(void) const { return mRss; }

    /**
     * This method returns the frame/radio Link Quality Indicator (LQI) value.
     *
     * @returns The Link Quality Indicator value.
     *
     */
    uint8_t GetLqi(void) const { return mLqi; }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    /**
     * This method returns the Time Sync Sequence.
     *
     * @returns The Time Sync Sequence.
     *
     */
    uint8_t GetTimeSyncSeq(void) const { return mTimeSyncSeq; }

    /**
     * This method returns the time offset to the Thread network time (in microseconds).
     *
     * @returns The time offset to the Thread network time (in microseconds).
     *
     */
    int64_t GetNetworkTimeOffset(void) const { return mNetworkTimeOffset; }
#endif

    /**
     * This method sets the `ThreadLinkInfo` from a given received frame.
     *
     * @param[in] aFrame  A received frame.
     *
     */
    void SetFrom(const Mac::RxFrame &aFrame);
};

/**
 * This class implements mesh forwarding within Thread.
 *
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
     * This constructor initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit MeshForwarder(Instance &aInstance);

    /**
     * This method enables mesh forwarding and the IEEE 802.15.4 MAC layer.
     *
     */
    void Start(void);

    /**
     * This method disables mesh forwarding and the IEEE 802.15.4 MAC layer.
     *
     */
    void Stop(void);

    /**
     * This method submits a message to the mesh forwarder for forwarding.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval kErrorNone     Successfully enqueued the message.
     * @retval kErrorAlready  The message was already enqueued.
     * @retval kErrorDrop     The message could not be sent and should be dropped.
     *
     */
    Error SendMessage(Message &aMessage);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    /**
     * This method sends an empty data frame to the parent.
     *
     * @retval kErrorNone          Successfully enqueued an empty message.
     * @retval kErrorInvalidState  Device is not in Rx-Off-When-Idle mode or it has no parent.
     * @retval kErrorNoBufs        Insufficient message buffers available.
     *
     */
    Error SendEmptyMessage(void);
#endif

    /**
     * This method is called by the address resolver when an EID-to-RLOC mapping has been resolved.
     *
     * @param[in]  aEid    A reference to the EID that has been resolved.
     * @param[in]  aError  kErrorNone on success and kErrorDrop otherwise.
     *
     */
    void HandleResolved(const Ip6::Address &aEid, Error aError);

    /**
     * This method indicates whether or not rx-on-when-idle mode is enabled.
     *
     * @retval TRUE   The rx-on-when-idle mode is enabled.
     * @retval FALSE  The rx-on-when-idle-mode is disabled.
     *
     */
    bool GetRxOnWhenIdle(void) const;

    /**
     * This method sets the rx-on-when-idle mode
     *
     * @param[in]  aRxOnWhenIdle  TRUE to enable, FALSE otherwise.
     *
     */
    void SetRxOnWhenIdle(bool aRxOnWhenIdle);

    /**
     * This method sets the scan parameters for MLE Discovery Request messages.
     *
     * @param[in]  aScanChannels  A reference to channel mask indicating which channels to scan.
     *                            If @p aScanChannels is empty, then all channels are used instead.
     *
     */
    void SetDiscoverParameters(const Mac::ChannelMask &aScanChannels);

#if OPENTHREAD_FTD
    /**
     * This method frees any messages queued for an existing child.
     *
     * @param[in]  aChild    A reference to the child.
     * @param[in]  aSubType  The message sub-type to remove.
     *                       Use Message::kSubTypeNone remove all messages for @p aChild.
     *
     */
    void RemoveMessages(Child &aChild, Message::SubType aSubType);
#endif

    /**
     * This method frees unicast/multicast MLE Data Responses from Send Message Queue if any.
     *
     */
    void RemoveDataResponseMessages(void);

    /**
     * This method evicts the message with lowest priority in the send queue.
     *
     * @param[in]  aPriority  The highest priority level of the evicted message.
     *
     * @retval kErrorNone       Successfully evicted a low priority message.
     * @retval kErrorNotFound   No low priority messages available to evict.
     *
     */
    Error EvictMessage(Message::Priority aPriority);

    /**
     * This method returns a reference to the send queue.
     *
     * @returns  A reference to the send queue.
     *
     */
    const PriorityQueue &GetSendQueue(void) const { return mSendQueue; }

    /**
     * This method returns a reference to the reassembly queue.
     *
     * @returns  A reference to the reassembly queue.
     *
     */
    const MessageQueue &GetReassemblyQueue(void) const { return mReassemblyList; }

    /**
     * This method returns a reference to the IP level counters.
     *
     * @returns A reference to the IP level counters.
     *
     */
    const otIpCounters &GetCounters(void) const { return mIpCounters; }

    /**
     * This method resets the IP level counters.
     *
     */
    void ResetCounters(void) { memset(&mIpCounters, 0, sizeof(mIpCounters)); }

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    /**
     * This method handles a deferred ack.
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
     *
     */
    void HandleDeferredAck(Neighbor &aNeighbor, Error aError);
#endif

private:
    static constexpr uint8_t kReassemblyTimeout      = OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT; // in seconds.
    static constexpr uint8_t kMeshHeaderFrameMtu     = OT_RADIO_FRAME_MAX_SIZE; // Max MTU with a Mesh Header frame.
    static constexpr uint8_t kMeshHeaderFrameFcsSize = sizeof(uint16_t);        // Frame FCS size for Mesh Header frame.

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

    enum AnycastType : uint8_t
    {
        kAnycastDhcp6Agent,
        kAnycastNeighborDiscoveryAgent,
        kAnycastService,
    };

#if OPENTHREAD_FTD
    class FragmentPriorityList : public Clearable<FragmentPriorityList>
    {
    public:
        class Entry : public Clearable<Entry>
        {
            friend class FragmentPriorityList;

        public:
            // Lifetime of an entry in seconds.
            static constexpr uint8_t kLifetime =
#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
                OT_MAX(kReassemblyTimeout, OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_FRAG_TAG_RETAIN_TIME);
#else
                kReassemblyTimeout;
#endif

            Message::Priority GetPriority(void) const { return static_cast<Message::Priority>(mPriority); }
            bool              IsExpired(void) const { return (mLifetime == 0); }
            void              DecrementLifetime(void) { mLifetime--; }
            void              ResetLifetime(void) { mLifetime = kLifetime; }

            bool Matches(uint16_t aSrcRloc16, uint16_t aTag) const
            {
                return (mSrcRloc16 == aSrcRloc16) && (mDatagramTag == aTag);
            }

#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
            bool ShouldDrop(void) const { return mShouldDrop; }
            void MarkToDrop(void) { mShouldDrop = true; }
#endif

        private:
            uint16_t mSrcRloc16;
            uint16_t mDatagramTag;
            uint8_t  mLifetime;
            uint8_t  mPriority : 2;
#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
            bool mShouldDrop : 1;
#endif

            static_assert(Message::kNumPriorities <= 4, "mPriority as a 2-bit does not fit all `Priority` values");
        };

        Entry *AllocateEntry(uint16_t aSrcRloc16, uint16_t aTag, Message::Priority aPriority);
        Entry *FindEntry(uint16_t aSrcRloc16, uint16_t aTag);
        bool   UpdateOnTimeTick(void);

    private:
        static constexpr uint16_t kNumEntries =
#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
            OT_MAX(OPENTHREAD_CONFIG_NUM_FRAGMENT_PRIORITY_ENTRIES,
                   OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_FRAG_TAG_ENTRY_LIST_SIZE);
#else
            OPENTHREAD_CONFIG_NUM_FRAGMENT_PRIORITY_ENTRIES;
#endif

        Entry mEntries[kNumEntries];
    };
#endif // OPENTHREAD_FTD

    void     SendIcmpErrorIfDstUnreach(const Message &     aMessage,
                                       const Mac::Address &aMacSource,
                                       const Mac::Address &aMacDest);
    Error    CheckReachability(const FrameData &   aFrameData,
                               const Mac::Address &aMeshSource,
                               const Mac::Address &aMeshDest);
    void     UpdateRoutes(const FrameData &aFrameData, const Mac::Address &aMeshSource, const Mac::Address &aMeshDest);
    Error    FrameToMessage(const FrameData &   aFrameData,
                            uint16_t            aDatagramSize,
                            const Mac::Address &aMacSource,
                            const Mac::Address &aMacDest,
                            Message *&          aMessage);
    void     GetMacDestinationAddress(const Ip6::Address &aIp6Addr, Mac::Address &aMacAddr);
    void     GetMacSourceAddress(const Ip6::Address &aIp6Addr, Mac::Address &aMacAddr);
    Message *PrepareNextDirectTransmission(void);
    void     HandleMesh(FrameData &aFrameData, const Mac::Address &aMacSource, const ThreadLinkInfo &aLinkInfo);
    void     HandleFragment(FrameData &           aFrameData,
                            const Mac::Address &  aMacSource,
                            const Mac::Address &  aMacDest,
                            const ThreadLinkInfo &aLinkInfo);
    void     HandleLowpanHC(const FrameData &     aFrameData,
                            const Mac::Address &  aMacSource,
                            const Mac::Address &  aMacDest,
                            const ThreadLinkInfo &aLinkInfo);
    uint16_t PrepareDataFrame(Mac::TxFrame &      aFrame,
                              Message &           aMessage,
                              const Mac::Address &aMacSource,
                              const Mac::Address &aMacDest,
                              bool                aAddMeshHeader = false,
                              uint16_t            aMeshSource    = 0xffff,
                              uint16_t            aMeshDest      = 0xffff,
                              bool                aAddFragHeader = false);
    void     PrepareEmptyFrame(Mac::TxFrame &aFrame, const Mac::Address &aMacDest, bool aAckRequest);

#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
    Error UpdateEcnOrDrop(Message &aMessage, bool aPreparingToSend = true);
    Error RemoveAgedMessages(void);
#endif
#if (OPENTHREAD_CONFIG_MAX_FRAMES_IN_DIRECT_TX_QUEUE > 0)
    bool IsDirectTxQueueOverMaxFrameThreshold(void) const;
    void ApplyDirectTxQueueLimit(Message &aMessage);
#endif
    void  SendMesh(Message &aMessage, Mac::TxFrame &aFrame);
    void  SendDestinationUnreachable(uint16_t aMeshSource, const Ip6::Headers &aIp6Headers);
    Error UpdateIp6Route(Message &aMessage);
    Error UpdateIp6RouteFtd(Ip6::Header &ip6Header, Message &aMessage);
    void  EvaluateRoutingCost(uint16_t aDest, uint8_t &aBestCost, uint16_t &aBestDest) const;
    Error AnycastRouteLookup(uint8_t aServiceId, AnycastType aType, uint16_t &aMeshDest) const;
    Error UpdateMeshRoute(Message &aMessage);
    bool  UpdateReassemblyList(void);
    void  UpdateFragmentPriority(Lowpan::FragmentHeader &aFragmentHeader,
                                 uint16_t                aFragmentLength,
                                 uint16_t                aSrcRloc16,
                                 Message::Priority       aPriority);
    Error HandleDatagram(Message &aMessage, const ThreadLinkInfo &aLinkInfo, const Mac::Address &aMacSource);
    void  ClearReassemblyList(void);
    void  RemoveMessage(Message &aMessage);
    void  HandleDiscoverComplete(void);

    void          HandleReceivedFrame(Mac::RxFrame &aFrame);
    Mac::TxFrame *HandleFrameRequest(Mac::TxFrames &aTxFrames);
    Neighbor *    UpdateNeighborOnSentFrame(Mac::TxFrame &      aFrame,
                                            Error               aError,
                                            const Mac::Address &aMacDest,
                                            bool                aIsDataPoll = false);
    void          UpdateNeighborLinkFailures(Neighbor &aNeighbor,
                                             Error     aError,
                                             bool      aAllowNeighborRemove,
                                             uint8_t   aFailLimit = Mle::kFailedRouterTransmissions);
    void          HandleSentFrame(Mac::TxFrame &aFrame, Error aError);
    void          UpdateSendMessage(Error aFrameTxError, Mac::Address &aMacDest, Neighbor *aNeighbor);
    void          RemoveMessageIfNoPendingTx(Message &aMessage);

    void        HandleTimeTick(void);
    static void ScheduleTransmissionTask(Tasklet &aTasklet);
    void        ScheduleTransmissionTask(void);

    Error GetFramePriority(const FrameData &   aFrameData,
                           const Mac::Address &aMacSource,
                           const Mac::Address &aMacDest,
                           Message::Priority & aPriority);
    Error GetFragmentPriority(Lowpan::FragmentHeader &aFragmentHeader,
                              uint16_t                aSrcRloc16,
                              Message::Priority &     aPriority);
    void  GetForwardFramePriority(const FrameData &   aFrameData,
                                  const Mac::Address &aMeshSource,
                                  const Mac::Address &aMeshDest,
                                  Message::Priority & aPriority);

    bool     CalcIePresent(const Message *aMessage);
    uint16_t CalcFrameVersion(const Neighbor *aNeighbor, bool aIePresent);
#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    void AppendHeaderIe(const Message *aMessage, Mac::TxFrame &aFrame);
#endif

    void PauseMessageTransmissions(void) { mTxPaused = true; }
    void ResumeMessageTransmissions(void);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_ENABLE
    static void HandleTxDelayTimer(Timer &aTimer);
    void        HandleTxDelayTimer(void);
#endif

    void LogMessage(MessageAction       aAction,
                    const Message &     aMessage,
                    Error               aError   = kErrorNone,
                    const Mac::Address *aAddress = nullptr);
    void LogFrame(const char *aActionText, const Mac::Frame &aFrame, Error aError);
    void LogFragmentFrameDrop(Error                         aError,
                              uint16_t                      aFrameLength,
                              const Mac::Address &          aMacSource,
                              const Mac::Address &          aMacDest,
                              const Lowpan::FragmentHeader &aFragmentHeader,
                              bool                          aIsSecure);
    void LogLowpanHcFrameDrop(Error               aError,
                              uint16_t            aFrameLength,
                              const Mac::Address &aMacSource,
                              const Mac::Address &aMacDest,
                              bool                aIsSecure);

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE)
    const char *MessageActionToString(MessageAction aAction, Error aError);
    const char *MessagePriorityToString(const Message &aMessage);

#if OPENTHREAD_FTD
    Error LogMeshFragmentHeader(MessageAction       aAction,
                                const Message &     aMessage,
                                const Mac::Address *aMacAddress,
                                Error               aError,
                                uint16_t &          aOffset,
                                Mac::Address &      aMeshSource,
                                Mac::Address &      aMeshDest,
                                LogLevel            aLogLevel);
    void  LogMeshIpHeader(const Message &     aMessage,
                          uint16_t            aOffset,
                          const Mac::Address &aMeshSource,
                          const Mac::Address &aMeshDest,
                          LogLevel            aLogLevel);
    void  LogMeshMessage(MessageAction       aAction,
                         const Message &     aMessage,
                         const Mac::Address *aAddress,
                         Error               aError,
                         LogLevel            aLogLevel);
#endif
    void LogIp6SourceDestAddresses(const Ip6::Headers &aHeaders, LogLevel aLogLevel);
    void LogIp6Message(MessageAction       aAction,
                       const Message &     aMessage,
                       const Mac::Address *aAddress,
                       Error               aError,
                       LogLevel            aLogLevel);
#endif // #if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE)

    PriorityQueue mSendQueue;
    MessageQueue  mReassemblyList;
    uint16_t      mFragTag;
    uint16_t      mMessageNextOffset;

    Message *mSendMessage;

    Mac::Address mMacSource;
    Mac::Address mMacDest;
    uint16_t     mMeshSource;
    uint16_t     mMeshDest;
    bool         mAddMeshHeader : 1;
    bool         mEnabled : 1;
    bool         mTxPaused : 1;
    bool         mSendBusy : 1;
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_ENABLE
    bool       mDelayNextTx : 1;
    TimerMilli mTxDelayTimer;
#endif

    Tasklet mScheduleTransmissionTask;

    otIpCounters mIpCounters;

#if OPENTHREAD_FTD
    FragmentPriorityList mFragmentPriorityList;
    IndirectSender       mIndirectSender;
#endif

    DataPollSender mDataPollSender;
};

/**
 * @}
 *
 */

DefineCoreType(otThreadLinkInfo, ThreadLinkInfo);

} // namespace ot

#endif // MESH_FORWARDER_HPP_
