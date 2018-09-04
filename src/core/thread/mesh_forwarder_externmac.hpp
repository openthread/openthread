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

#ifndef MESH_FORWARDER_EXTERNMAC_HPP_
#define MESH_FORWARDER_EXTERNMAC_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_USE_EXTERNAL_MAC

#include "common/locator.hpp"
#include "common/tasklet.hpp"
#include "mac/mac_extern.hpp"
#include "net/ip6.hpp"
#include "thread/address_resolver.hpp"
#include "thread/data_poll_manager.hpp"
#include "thread/lowpan.hpp"
#include "thread/network_data_leader.hpp"
#include "thread/src_match_controller.hpp"
#include "thread/topology.hpp"

namespace ot {

enum
{
    kReassemblyTimeout = OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT,
};

class MleRouter;
class MeshForwarder;

/**
 * @addtogroup core-mesh-forwarding
 *
 * @brief
 *   This module includes definitions for mesh forwarding within Thread.
 *
 * @{
 */

class MeshSender
{
    friend MeshForwarder;

public:
    MeshSender(void);

private:
    otInstance *GetInstance(void);

    /*
     * These methods handle the Mac layer requesting a frame from this sender.
     * The static function acts to resolve the context to a MeshSender object
     * which will handle the sending. This method fills the frame with data
     * before returning.
     */
    static otError DispatchFrameRequest(Mac::Sender &aSender, Mac::Frame &aFrame, otDataRequest &aDataReq);
    otError        HandleFrameRequest(Mac::Sender &aSender, Mac::Frame &aFrame, otDataRequest &aDataReq);

    /*
     * These methods handle the Mac layer completing the sending of a frame.
     * The MAC layer will call these functions for the same Sender that
     * handled the frame request. This allows the sender to prepare the next
     * frame for transmission.
     */
    static void DispatchSentFrame(Mac::Sender &aSender, otError aError);
    void        HandleSentFrame(Mac::Sender &aSender, otError aError);

    /*
     * These are internal functions to handle the construction of certain mac frames.
     */
    void    PrepareIndirectTransmission(const Child &aChild);
    otError ScheduleIndirectTransmission();
    otError ScheduleDirectTransmission();

    otError SendMesh(Message &aMessage, otDataRequest &aDataReq);
    otError SendFragment(Message &aMessage, Mac::Frame &aFrame, otDataRequest &aDataReq);
    otError SendIdleFrame(otDataRequest &aDataReq);
    otError SendEmptyFrame(bool aAckRequest, otDataRequest &aDataReq);

    size_t GetMaxMsduSize(otDataRequest &aDataReq);

    bool isDirectSender() { return (mBoundChild == NULL); }

    Mac::Sender    mSender;
    uint16_t       mMessageNextOffset;
    Message *      mSendMessage;
    uint16_t       mMeshSource;
    uint16_t       mMeshDest;
    bool           mAddMeshHeader;
    MeshForwarder *mParent;
    bool           mAckRequested;
    bool           mIdleMessageSent;

    Mac::Address mMacSource;
    Mac::Address mMacDest;

    Child *mBoundChild;
};

/**
 * This class implements mesh forwarding within Thread.
 *
 */
class MeshForwarder : public InstanceLocator
{
    friend MeshSender;

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
     * @retval OT_ERROR_NONE          Successfully enabled the mesh forwarder.
     *
     */
    otError Start(void);

    /**
     * This method disables mesh forwarding and the IEEE 802.15.4 MAC layer.
     *
     * @retval OT_ERROR_NONE          Successfully disabled the mesh forwarder.
     *
     */
    otError Stop(void);

    /**
     * This method submits a message to the mesh forwarder for forwarding.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully enqueued the message.
     * @retval OT_ERROR_ALREADY  The message was already enqueued.
     * @retval OT_ERROR_DROP     The message could not be sent and should be dropped.
     *
     */
    otError SendMessage(Message &aMessage);

    /**
     * This method Sends an 802.15.4 poll to the parent
     *
     * @retval OT_ERROR_NONE     Successfully sent poll
     * @retval OT_ERROR_BUSY     The MAC is not in the correct state
     * @retval OT_ERROR_NO_ACK   The Poll was not acknowledged by the parent
     *
     */
    otError SendPoll();

    /**
     * This method is called by the address resolver when an EID-to-RLOC mapping has been resolved.
     *
     * @param[in]  aEid    A reference to the EID that has been resolved.
     * @param[in]  aError  OT_ERROR_NONE on success and OT_ERROR_DROP otherwise.
     *
     */
    void HandleResolved(const Ip6::Address &aEid, otError aError);

    /**
     * This method sets the radio receiver and polling timer off.
     *
     */
    void SetRxOff(void);

    /**
     * This method indicates whether or not rx-on-when-idle mode is enabled.
     *
     * @retval TRUE   The rx-on-when-idle mode is enabled.
     * @retval FALSE  The rx-on-when-idle-mode is disabled.
     *
     */
    bool GetRxOnWhenIdle(void);

    /**
     * This method sets the rx-on-when-idle mode
     *
     * @param[in]  aRxOnWhenIdle  TRUE to enable, FALSE otherwise.
     *
     */
    void SetRxOnWhenIdle(bool aRxOnWhenIdle);

    /**
     * This method returns the number of SED Slots that are unused
     *
     * @returns the number of SED Slots that are free
     */
    uint8_t GetRemainingSEDSlotCount(void);

    /**
     * This method binds a new SED to a SED slot for indirect messaging
     *
     * @param[in]  aChild  A reference to the child to be allocated a SED slot
     *
     * @retval OT_ERROR_NONE     Slot successfully bound
     * @retval OT_ERROR_NO_BUFS  No available slot
     */
    otError AllocateSEDSlot(Child &aChild);

    /**
     * This method unbinds a SED from a SED slot
     *
     * @param[in]  aChild  A reference to the child to be deallocated
     *
     * @retval OT_ERROR_NONE     Slot successfully deallocated
     */
    otError DeallocateSEDSlot(Child &aChild);

    /**
     * This method sets the scan parameters for MLE Discovery Request messages.
     *
     * @param[in]  aScanChannels  A bit vector indicating which channels to scan.
     *
     */
    void SetDiscoverParameters(uint32_t aScanChannels);

    /**
     * This method frees any indirect messages queued for a specific child.
     *
     * @param[in]  aChild  A reference to a child whom messages shall be removed.
     *
     */
    void ClearChildIndirectMessages(Child &aChild);

    /**
     * This method frees any indirect messages queued for children that are no longer attached.
     *
     */
    void UpdateIndirectMessages(void);

    /**
     * This method frees any messages queued for an existing child.
     *
     * @param[in]  aChild    A reference to the child.
     * @param[in]  aSubType  The message sub-type to remove.
     *                       Use Message::kSubTypeNone remove all messages for @p aChild.
     *
     */
    void RemoveMessages(Child &aChild, uint8_t aSubType);

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
     * @retval OT_ERROR_NONE       Successfully evicted a low priority message.
     * @retval OT_ERROR_NOT_FOUND  No low priority messages available to evict.
     *
     */
    otError EvictMessage(uint8_t aPriority);

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
     * This method returns a reference to the data poll manager.
     *
     * @returns  A reference to the data poll manager.
     *
     */
    DataPollManager &GetDataPollManager(void) { return mDataPollManager; }

    /**
     * This method returns a reference to the IP level counters.
     *
     * @returns A reference to the IP level counters.
     *
     */
    const otIpCounters &GetCounters(void) const { return mIpCounters; }

#if OPENTHREAD_FTD
    /**
     * This method returns a reference to the resolving queue.
     *
     * @returns  A reference to the resolving queue.
     *
     */
    const MessageQueue &GetResolvingQueue(void) const { return mResolvingQueue; }

    /**
     * This method returns a reference to the source match controller.
     *
     * @returns  A reference to the source match controller.
     *
     */
    SourceMatchController &GetSourceMatchController(void) { return mSourceMatchController; }
#endif

private:
    enum
    {
        kStateUpdatePeriod  = 1000,                                            ///< State update period in milliseconds.
        kNumIndirectSenders = OPENTHREAD_CONFIG_EXTERNAL_MAC_MAX_SEDS,         ///< Indirects reserved for single SEDs
        kNumFloatingSenders = OPENTHREAD_CONFIG_EXTERNAL_MAC_FLOATING_SENDERS, ///< Shared senders
    };

    enum
    {
        /**
         * Maximum number of tx attempts by `MeshForwarder` for an outbound indirect frame (for a sleepy child). The
         * `MeshForwader` attempts occur following the reception of a new data request command (a new data poll) from
         * the sleepy child.
         *
         */
        kMaxPollTriggeredTxAttempts = OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_INDIRECT_POLLS,

        /**
         * Indicates whether to set/enable 15.4 ack request in the MAC header of a supervision message.
         *
         */
        kSupervisionMsgAckRequest = (OPENTHREAD_CONFIG_SUPERVISION_MSG_NO_ACK_REQUEST == 0) ? true : false,
    };

    enum MessageAction ///< Defines the action parameter in `LogMessageInfo()` method.
    {
        kMessageReceive,         ///< Indicates that the message was received.
        kMessageTransmit,        ///< Indicates that the message was sent.
        kMessagePrepareIndirect, ///< Indicates that the message is being prepared for indirect tx.
        kMessageDrop,            ///< Indicates that the outbound message is being dropped (e.g., dst unknown).
        kMessageReassemblyDrop,  ///< Indicates that the message is being dropped from reassembly list.
        kMessageEvict,           ///< Indicates that the message was evicted.
    };

    otError  CheckReachability(uint8_t *           aFrame,
                               uint8_t             aFrameLength,
                               const Mac::Address &aMeshSource,
                               const Mac::Address &aMeshDest);
    void     UpdateRoutes(uint8_t *           aFrame,
                          uint8_t             aFrameLength,
                          const Mac::Address &aMeshSource,
                          const Mac::Address &aMeshDest);
    otError  SkipMeshHeader(const uint8_t *&aFrame, uint8_t &aFrameLength);
    otError  DecompressIp6Header(const uint8_t *     aFrame,
                                 uint8_t             aFrameLength,
                                 const Mac::Address &aMacSource,
                                 const Mac::Address &aMacDest,
                                 Ip6::Header &       aIp6Header,
                                 uint8_t &           aHeaderLength,
                                 bool &              aNextHeaderCompressed);
    otError  GetIp6Header(const uint8_t *     aFrame,
                          uint8_t             aFrameLength,
                          const Mac::Address &aMacSource,
                          const Mac::Address &aMacDest,
                          Ip6::Header &       aIp6Header);
    otError  GetMacDestinationAddress(const Ip6::Address &aIp6Addr, Mac::Address &aMacAddr);
    otError  GetMacSourceAddress(const Ip6::Address &aIp6Addr, Mac::Address &aMacAddr);
    Message *GetDirectTransmission(MeshSender &aSender);
    Message *GetIndirectTransmission(Child &aChild);
    otError  PrepareDiscoverRequest(void);
    otError  PrepareDataPoll(void);
    void     HandleMesh(uint8_t *               aFrame,
                        uint8_t                 aPayloadLength,
                        const Mac::Address &    aMacSource,
                        const otThreadLinkInfo &aLinkInfo);
    void     HandleFragment(uint8_t *               aFrame,
                            uint8_t                 aPayloadLength,
                            const Mac::Address &    aMacSource,
                            const Mac::Address &    aMacDest,
                            const otThreadLinkInfo &aLinkInfo);
    void     HandleLowpanHC(uint8_t *               aFrame,
                            uint8_t                 aPayloadLength,
                            const Mac::Address &    aMacSource,
                            const Mac::Address &    aMacDest,
                            const otThreadLinkInfo &aLinkInfo);
    otError  UpdateIp6Route(Message &aMessage, MeshSender &aSender);
    otError  UpdateMeshRoute(Message &aMessage, MeshSender &aSender);
    otError  HandleDatagram(Message &aMessage, const otThreadLinkInfo &aLinkInfo, const Mac::Address &aMacSource);
    void     ClearReassemblyList(void);
    otError  RemoveMessageFromSleepyChild(Message &aMessage, Child &aChild);
    void     RemoveMessage(Message &aMessage);

    static void HandleReceivedFrame(Mac::Receiver &aReceiver, otDataIndication &aDataIndication);
    void        HandleReceivedFrame(otDataIndication &aDataIndication);
    static void HandleDiscoverTimer(Timer &aTimer);
    void        HandleDiscoverTimer(void);
    static void HandleReassemblyTimer(Timer &aTimer);
    void        HandleReassemblyTimer(void);
    static void ScheduleTransmissionTask(Tasklet &aTasklet);
    void        ScheduleTransmissionTask(void);
    static void HandleDataPollTimeout(Mac::Receiver &aReceiver);

    // Functions to claim/release Floating Mac Senders
    Mac::Sender *GetFreeFloatingSender(MeshSender *aSender);  // Get and claim an unclaimed sender
    Mac::Sender *GetIdleFloatingSender(MeshSender *aSender);  // Get sender claimed for aSender & idle
    void         ReleaseFloatingSenders(MeshSender *aSender); // Release claim to all senders

    uint16_t GetNextFragTag(void);

#if OPENTHREAD_FTD
#if OPENTHREAD_ENABLE_SERVICE
    otError GetDestinationRlocByServiceAloc(uint16_t aServiceAloc, uint16_t &aMeshDest);
#endif // OPENTHREAD_ENABLE_SERVICE
#endif // OPENTHREAD_FTD

    void LogIp6Message(MessageAction aAction, const Message &aMessage, const Mac::Address *aMacAddress, otError aError);

    Mac::Receiver mMacReceiver;
    TimerMilli    mDiscoverTimer;
    TimerMilli    mReassemblyTimer;

    PriorityQueue mSendQueue;
    // WARNING: The MeshForwarder is very tightly coupled with the 'directSender'
    MeshSender mDirectSender;

    /*
     * The mOverflowMacSender is used for indirectly sending ip frames that take up more than 1 15.4 message. If a
     * sender needs to do this, it should 'Claim' the overflow by pointing it to itself. It can only be claimed if it is
     * not null. After it is done using the overflow potential, it should set the pointer back to NULL. This is used to
     * control how much of the external MAC's resources each SED is permitted to consume.
     */
#if OPENTHREAD_CONFIG_EXTERNAL_MAC_FLOATING_SENDERS != 0
    Mac::Sender mFloatingMacSenders[kNumFloatingSenders];
#else
    Mac::Sender *mFloatingMacSenders;
#endif

#if OPENTHREAD_CONFIG_EXTERNAL_MAC_MAX_SEDS != 0
    MeshSender mMeshSenders[kNumIndirectSenders];
#else
    MeshSender * mMeshSenders;
#endif

    MessageQueue mReassemblyList;
    MessageQueue mResolvingQueue;

    Tasklet  mScheduleTransmissionTask;
    bool     mEnabled;
    uint16_t mFragTag;

    // For use only with the direct sender
    uint32_t mScanChannels;
    uint8_t  mScanChannel;
    uint8_t  mRestoreChannel;
    uint16_t mRestorePanId;
    bool     mScanning;

    DataPollManager       mDataPollManager;
    SourceMatchController mSourceMatchController;

    otIpCounters mIpCounters;
};

/**
 * @}
 *
 */

} // namespace ot

#endif // OPENTHREAD_CONFIG_USE_EXTERNAL_MAC

#endif // MESH_FORWARDER_HPP_
