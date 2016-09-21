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

#include <openthread-core-config.h>
#include <openthread-types.h>
#include <common/tasklet.hpp>
#include <mac/mac.hpp>
#include <net/ip6.hpp>
#include <thread/address_resolver.hpp>
#include <thread/lowpan.hpp>
#include <thread/network_data_leader.hpp>
#include <thread/topology.hpp>

namespace Thread {

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
     * This method enables mesh forwarding and the IEEE 802.15.4 MAC layer.
     *
     * @retval kThreadError_None          Successfully enabled the mesh forwarder.
     * @retval kThreadError_InvalidState  The mesh forwarder was already enabled.
     *
     */
    ThreadError Start(void);

    /**
     * This method disables mesh forwarding and the IEEE 802.15.4 MAC layer.
     *
     * @retval kThreadError_None          Successfully disabled the mesh forwarder.
     * @retval kThreadError_InvalidState  The mesh forwarder was already disabled.
     *
     */
    ThreadError Stop(void);

    /**
     * This method submits a message to the mesh forwarder for forwarding.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval kThreadError_None  Successfully enqueued the message.
     *
     */
    ThreadError SendMessage(Message &aMessage);

    /**
     * This method is called by the address resolver when an EID-to-RLOC mapping has been resolved.
     *
     * @param[in]  aEid    A reference to the EID that has been resolved.
     * @param[in]  aError  kThreadError_None on success and kThreadError_Drop otherwise.
     *
     */
    void HandleResolved(const Ip6::Address &aEid, ThreadError aError);

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
     * This method sets customized Data Poll period. Only for certification test
     *
     * @param[in]  aPeriod  The Data Poll period in milliseconds.
     *
     */
    void SetAssignPollPeriod(uint32_t aPeriod);

    /**
     * This method gets the customized Data Poll period. Only for certification test
     *
     * @returns  The Data Poll period in milliseconds.
     *
     */
    uint32_t GetAssignPollPeriod(void);

    /**
     * This method sets the Data Poll period.
     *
     * @param[in]  aPeriod  The Data Poll period in milliseconds.
     *
     */
    void SetPollPeriod(uint32_t aPeriod);

    /**
     * This method gets the Data Poll period.
     *
     * @returns  The Data Poll period in milliseconds.
     *
     */
    uint32_t GetPollPeriod(void);

    /**
     * This method sets the scan parameters for MLE Discovery Request messages.
     *
     * @param[in]  aScanChannels  A bit vector indicating which channels to scan.
     * @param[in]  aScanDuration  The time in milliseconds to spend scanning each channel.
     *
     */
    void SetDiscoverParameters(uint32_t aScanChannels, uint16_t aScanDuration);

private:
    enum
    {
        kStateUpdatePeriod = 1000,  ///< State update period in milliseconds.
    };

    ThreadError CheckReachability(uint8_t *aFrame, uint8_t aFrameLength,
                                  const Mac::Address &aMeshSource, const Mac::Address &aMeshDest);
    ThreadError GetMacDestinationAddress(const Ip6::Address &aIp6Addr, Mac::Address &aMacAddr);
    ThreadError GetMacSourceAddress(const Ip6::Address &aIp6Addr, Mac::Address &aMacAddr);
    Message *GetDirectTransmission(void);
    Message *GetIndirectTransmission(const Child &aChild);
    void HandleMesh(uint8_t *aFrame, uint8_t aPayloadLength, const ThreadMessageInfo &aMessageInfo);
    void HandleFragment(uint8_t *aFrame, uint8_t aPayloadLength,
                        const Mac::Address &aMacSource, const Mac::Address &aMacDest,
                        const ThreadMessageInfo &aMessageInfo);
    void HandleLowpanHC(uint8_t *aFrame, uint8_t aPayloadLength,
                        const Mac::Address &aMacSource, const Mac::Address &aMacDest,
                        const ThreadMessageInfo &aMessageInfo);
    void HandleDataRequest(const Mac::Address &aMacSource, const ThreadMessageInfo &aMessageInfo);
    void MoveToResolving(const Ip6::Address &aDestination);
    ThreadError SendPoll(Message &aMessage, Mac::Frame &aFrame);
    ThreadError SendMesh(Message &aMessage, Mac::Frame &aFrame);
    ThreadError SendFragment(Message &aMessage, Mac::Frame &aFrame);
    void UpdateFramePending(void);
    ThreadError UpdateIp6Route(Message &aMessage);
    ThreadError UpdateMeshRoute(Message &aMessage);
    ThreadError HandleDatagram(Message &aMessage, const ThreadMessageInfo &aMessageInfo);

    static void HandleReceivedFrame(void *aContext, Mac::Frame &aFrame, ThreadError aError);
    void HandleReceivedFrame(Mac::Frame &aFrame, ThreadError aError);

    static ThreadError HandleFrameRequest(void *aContext, Mac::Frame &aFrame);
    ThreadError HandleFrameRequest(Mac::Frame &aFrame);

    static void HandleSentFrame(void *aContext, Mac::Frame &aFrame);
    void HandleSentFrame(Mac::Frame &aFrame);

    static void HandleDiscoverTimer(void *aContext);
    void HandleDiscoverTimer(void);
    static void HandleReassemblyTimer(void *aContext);
    void HandleReassemblyTimer(void);
    static void HandlePollTimer(void *aContext);
    void HandlePollTimer(void);

    static void ScheduleTransmissionTask(void *aContext);
    void ScheduleTransmissionTask(void);

    ThreadError AddPendingSrcMatchEntries();
    ThreadError AddSrcMatchEntry(Child &aChild);
    void ClearSrcMatchEntry(Child &aChild);

    Mac::Receiver mMacReceiver;
    Mac::Sender mMacSender;
    Timer mDiscoverTimer;
    Timer mPollTimer;
    Timer mReassemblyTimer;

    MessageQueue mSendQueue;
    MessageQueue mReassemblyList;
    MessageQueue mResolvingQueue;
    uint16_t mFragTag;
    uint16_t mMessageNextOffset;
    uint32_t mPollPeriod;
    uint32_t mAssignPollPeriod;  ///< only for certification test
    Message *mSendMessage;

    Mac::Address mMacSource;
    Mac::Address mMacDest;
    uint16_t mMeshSource;
    uint16_t mMeshDest;
    bool mAddMeshHeader;

    bool mSendBusy;

    Tasklet mScheduleTransmissionTask;
    bool mEnabled;

    uint32_t mScanChannels;
    uint16_t mScanDuration;
    uint8_t mScanChannel;
    uint8_t mRestoreChannel;
    bool mScanning;

    ThreadNetif &mNetif;
    AddressResolver &mAddressResolver;
    Lowpan::Lowpan &mLowpan;
    Mac::Mac &mMac;
    Mle::MleRouter &mMle;
    NetworkData::Leader &mNetworkData;

    bool mSrcMatchEnabled;
};

/**
 * @}
 *
 */

}  // namespace Thread

#endif  // MESH_FORWARDER_HPP_
