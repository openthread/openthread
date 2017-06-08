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

#include <openthread/types.h>

#include "openthread-core-config.h"
#include "common/tasklet.hpp"
#include "mac/mac.hpp"
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
struct ThreadMessageInfo;

/**
 * @addtogroup core-mesh-forwarding
 *
 * @brief
 *   This module includes definitions for mesh forwarding within Thread.
 *
 * @{
 */

/**
 * This class implements mesh forwarding within Thread.
 *
 */
class MeshForwarder
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aThreadNetif  A reference to the Thread network interface.
     *
     */
    explicit MeshForwarder(ThreadNetif &aThreadNetif);

    /**
     * This method returns the pointer to the parent otInstance structure.
     *
     * @returns The pointer to the parent otInstance structure.
     *
     */
    otInstance *GetInstance(void);

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
     * This method returns a reference to the thread network interface instance.
     *
     * @ returns   A reference to the thread network interface instance.
     */
    ThreadNetif &GetNetif(void) { return mNetif; }

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
     * This method returns a reference to the resolving queue.
     *
     * @returns  A reference to the resolving queue.
     *
     */
    const MessageQueue &GetResolvingQueue(void) const { return mResolvingQueue; }

    /**
     * This method returns a reference to the data poll manager.
     *
     * @returns  A reference to the data poll manager.
     *
     */
    DataPollManager &GetDataPollManager(void) { return mDataPollManager; }

    /**
     * This method returns a reference to the source match controller.
     *
     * @returns  A reference to the source match controller.
     *
     */
    SourceMatchController &GetSourceMatchController(void) { return mSourceMatchController; }

    /**
     * This method returns a reference to the IP level counters.
     *
     * @returns A reference to the IP level counters.
     *
     */
    const otIpCounters &GetCounters(void) { return mIpCounters; }
private:
    enum
    {
        kStateUpdatePeriod     = 1000,  ///< State update period in milliseconds.
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
        kSupervisionMsgAckRequest   = (OPENTHREAD_CONFIG_SUPERVISION_MSG_NO_ACK_REQUEST == 0) ? true : false,
    };

    enum MessageAction                   ///< Defines the action parameter in `LogMessageInfo()` method.
    {
        kMessageReceive,                 ///< Indicates that the message was received.
        kMessageTransmit,                ///< Indicates that the message was sent.
        kMessagePrepareIndirect,         ///< Indicates that the message is being prepared for indirect tx.
        kMessageDrop,                    ///< Indicates that the message is being dropped from reassembly list.
    };

    otError CheckReachability(uint8_t *aFrame, uint8_t aFrameLength,
                              const Mac::Address &aMeshSource, const Mac::Address &aMeshDest);

    otError GetMacDestinationAddress(const Ip6::Address &aIp6Addr, Mac::Address &aMacAddr);
    otError GetMacSourceAddress(const Ip6::Address &aIp6Addr, Mac::Address &aMacAddr);
    Message *GetDirectTransmission(void);
    Message *GetIndirectTransmission(Child &aChild);
    otError PrepareDiscoverRequest(void);
    void PrepareIndirectTransmission(Message &aMessage, const Child &aChild);
    void HandleMesh(uint8_t *aFrame, uint8_t aPayloadLength, const Mac::Address &aMacSource,
                    const ThreadMessageInfo &aMessageInfo);
    void HandleFragment(uint8_t *aFrame, uint8_t aPayloadLength,
                        const Mac::Address &aMacSource, const Mac::Address &aMacDest,
                        const ThreadMessageInfo &aMessageInfo);
    void HandleLowpanHC(uint8_t *aFrame, uint8_t aPayloadLength,
                        const Mac::Address &aMacSource, const Mac::Address &aMacDest,
                        const ThreadMessageInfo &aMessageInfo);
    void HandleDataRequest(const Mac::Address &aMacSource, const ThreadMessageInfo &aMessageInfo);
    otError SendPoll(Message &aMessage, Mac::Frame &aFrame);
    otError SendMesh(Message &aMessage, Mac::Frame &aFrame);
    otError SendFragment(Message &aMessage, Mac::Frame &aFrame);
    otError SendEmptyFrame(Mac::Frame &aFrame, bool aAckRequest);
    otError UpdateIp6Route(Message &aMessage);
    otError UpdateMeshRoute(Message &aMessage);
    otError HandleDatagram(Message &aMessage, const ThreadMessageInfo &aMessageInfo,
                           const Mac::Address &aMacSource);
    void ClearReassemblyList(void);

    static void HandleReceivedFrame(void *aContext, Mac::Frame &aFrame);
    void HandleReceivedFrame(Mac::Frame &aFrame);
    static otError HandleFrameRequest(void *aContext, Mac::Frame &aFrame);
    otError HandleFrameRequest(Mac::Frame &aFrame);
    static void HandleSentFrame(void *aContext, Mac::Frame &aFrame, otError aError);
    void HandleSentFrame(Mac::Frame &aFrame, otError aError);
    static void HandleDiscoverTimer(void *aContext);
    void HandleDiscoverTimer(void);
    static void HandleReassemblyTimer(void *aContext);
    void HandleReassemblyTimer(void);
    static void ScheduleTransmissionTask(void *aContext);
    void ScheduleTransmissionTask(void);
    static void HandleDataPollTimeout(void *aContext);

    otError AddPendingSrcMatchEntries(void);
    otError AddSrcMatchEntry(Child &aChild);
    void ClearSrcMatchEntry(Child &aChild);

    void LogIp6Message(MessageAction aAction, const Message &aMessage, const Mac::Address *aMacAddress,
                       otError aError);

    
    ThreadNetif           &mNetif;

    Mac::Receiver         mMacReceiver;
    Mac::Sender           mMacSender;
    Timer                 mDiscoverTimer;
    Timer                 mReassemblyTimer;

    PriorityQueue         mSendQueue;
    MessageQueue          mReassemblyList;
    MessageQueue          mResolvingQueue;
    uint16_t              mFragTag;
    uint16_t              mMessageNextOffset;

    uint32_t              mSendMessageFrameCounter;
    Message               *mSendMessage;
    bool                  mSendMessageIsARetransmission;
    uint8_t               mSendMessageMaxMacTxAttempts;
    uint8_t               mSendMessageKeyId;
    uint8_t               mSendMessageDataSequenceNumber;
    uint8_t               mStartChildIndex;

    Mac::Address          mMacSource;
    Mac::Address          mMacDest;
    uint16_t              mMeshSource;
    uint16_t              mMeshDest;
    bool                  mAddMeshHeader;

    bool                  mSendBusy;

    Tasklet               mScheduleTransmissionTask;
    bool                  mEnabled;

    uint32_t              mScanChannels;
    uint8_t               mScanChannel;
    uint8_t               mRestoreChannel;
    uint16_t              mRestorePanId;
    bool                  mScanning;

    DataPollManager       mDataPollManager;
    SourceMatchController mSourceMatchController;

    otIpCounters          mIpCounters;
};

/**
 * @}
 *
 */

}  // namespace ot

#endif  // MESH_FORWARDER_HPP_
