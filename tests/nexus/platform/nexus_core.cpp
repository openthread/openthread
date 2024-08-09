/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include "nexus_core.hpp"
#include "nexus_node.hpp"

namespace ot {
namespace Nexus {

Core *Core::sCore = nullptr;

Core::Core(void)
    : mNow(0)
    , mCurNodeId(0)
    , mPendingAction(false)
{
    VerifyOrQuit(sCore == nullptr);
    sCore = this;

    mNextAlarmTime = mNow.GetDistantFuture();
}

Node &Core::CreateNode(void)
{
    Node *node;

    node = Node::Allocate();
    VerifyOrQuit(node != nullptr);

    node->GetInstance().SetId(mCurNodeId++);

    mNodes.Push(*node);

    node->GetInstance().AfterInit();

    return *node;
}

void Core::UpdateNextAlarmTime(const Alarm &aAlarm)
{
    if (aAlarm.mScheduled)
    {
        mNextAlarmTime = Min(mNextAlarmTime, Max(mNow, aAlarm.mAlarmTime));
    }
}

void Core::AdvanceTime(uint32_t aDuration)
{
    TimeMilli targetTime = mNow + aDuration;

    while (mPendingAction || (mNextAlarmTime <= targetTime))
    {
        mNextAlarmTime = mNow.GetDistantFuture();
        mPendingAction = false;

        for (Node &node : mNodes)
        {
            Process(node);
            UpdateNextAlarmTime(node.mAlarm);
        }

        if (!mPendingAction)
        {
            mNow = Min(mNextAlarmTime, targetTime);
        }
    }

    mNow = targetTime;
}

void Core::Process(Node &aNode)
{
    otTaskletsProcess(&aNode.GetInstance());

    ProcessRadio(aNode);

    if (aNode.mAlarm.ShouldTrigger(mNow))
    {
        otPlatAlarmMilliFired(&aNode.GetInstance());
    }
}

void Core::ProcessRadio(Node &aNode)
{
    Mac::Address dstAddr;
    uint16_t     dstPanId;
    bool         ackRequested;
    AckMode      ackMode = kNoAck;

    VerifyOrExit(aNode.mRadio.mState == Radio::kStateTransmit);

    if (aNode.mRadio.mTxFrame.GetDstAddr(dstAddr) != kErrorNone)
    {
        dstAddr.SetNone();
    }

    if (aNode.mRadio.mTxFrame.GetDstPanId(dstPanId) != kErrorNone)
    {
        dstPanId = Mac::kPanIdBroadcast;
    }

    ackRequested = aNode.mRadio.mTxFrame.GetAckRequest();

    otPlatRadioTxStarted(&aNode.GetInstance(), &aNode.mRadio.mTxFrame);

    for (Node &rxNode : mNodes)
    {
        bool matchesDst;

        if ((&rxNode == &aNode) || !rxNode.mRadio.CanReceiveOnChannel(aNode.mRadio.mTxFrame.GetChannel()))
        {
            continue;
        }

        matchesDst = rxNode.mRadio.Matches(dstAddr, dstPanId);

        if (matchesDst || rxNode.mRadio.mPromiscuous)
        {
            // `rxNode` should receive this frame.

            Radio::Frame rxFrame(aNode.mRadio.mTxFrame);

            rxFrame.mInfo.mRxInfo.mTimestamp = (mNow.GetValue() * 1000u);
            rxFrame.mInfo.mRxInfo.mRssi      = kDefaultRxRssi;
            rxFrame.mInfo.mRxInfo.mLqi       = 0;

            if (matchesDst && !dstAddr.IsNone() && !dstAddr.IsBroadcast() && ackRequested)
            {
                Mac::Address srcAddr;

                ackMode = kSendAckNoFramePending;

                if ((aNode.mRadio.mTxFrame.GetSrcAddr(srcAddr) == kErrorNone) &&
                    rxNode.mRadio.HasFramePendingFor(srcAddr))
                {
                    ackMode                                      = kSendAckFramePending;
                    rxFrame.mInfo.mRxInfo.mAckedWithFramePending = true;
                }
            }

            otPlatRadioReceiveDone(&rxNode.GetInstance(), &rxFrame, kErrorNone);
        }

        if (ackMode != kNoAck)
        {
            // No need to go through rest of `mNodes`
            // if already acked by a node.
            break;
        }
    }

    aNode.mRadio.mChannel = aNode.mRadio.mTxFrame.mChannel;
    aNode.mRadio.mState   = Radio::kStateReceive;

    if (ackMode != kNoAck)
    {
        Mac::TxFrame ackFrame;
        uint8_t      ackPsdu[Mac::Frame::kImmAckLength];

        ClearAllBytes(ackFrame);
        ackFrame.mPsdu = ackPsdu;

        ackFrame.GenerateImmAck(
            static_cast<const Mac::RxFrame &>(static_cast<const Mac::Frame &>(aNode.mRadio.mTxFrame)),
            (ackMode == kSendAckFramePending));

        otPlatRadioTxDone(&aNode.GetInstance(), &aNode.mRadio.mTxFrame, &ackFrame, kErrorNone);
    }
    else
    {
        otPlatRadioTxDone(&aNode.GetInstance(), &aNode.mRadio.mTxFrame, nullptr,
                          ackRequested ? kErrorNoAck : kErrorNone);
    }

exit:
    return;
}

} // namespace Nexus
} // namespace ot
