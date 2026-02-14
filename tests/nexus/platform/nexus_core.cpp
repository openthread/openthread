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

#include <cstdlib>
#include <cstring>

#include "nexus_json.hpp"
#include "nexus_node.hpp"

namespace ot {
namespace Nexus {

Core *Core::sCore  = nullptr;
bool  Core::sInUse = false;

Core::Core(void)
    : mCurNodeId(0)
    , mPendingAction(false)
    , mNow(0)
    , mActiveNode(nullptr)
{
    const char *pcapFile;

    VerifyOrQuit(!sInUse);
    sCore  = this;
    sInUse = true;

    mNextAlarmTime = mNow.GetDistantFuture();

    pcapFile = getenv("OT_NEXUS_PCAP_FILE");

    if ((pcapFile != nullptr) && (pcapFile[0] != '\0'))
    {
        mPcap.Open(pcapFile);
    }
}

void Core::SaveTestInfo(const char *aFileName)
{
    Json::Writer jsonWriter;

    SuccessOrExit(jsonWriter.OpenFile(aFileName));

    jsonWriter.WriteNameValue("testcase", Json::ExtractTestName(aFileName).AsCString());
    jsonWriter.WriteNameValue("pcap", getenv("OT_NEXUS_PCAP_FILE"));

    if (!mNodes.IsEmpty())
    {
        NetworkKey networkKey;

        mNodes.GetHead()->Get<KeyManager>().GetNetworkKey(networkKey);

        jsonWriter.WriteNameValue("network_key", networkKey.ToString().AsCString());

        for (Node &node : mNodes)
        {
            if (node.Get<Mle::Mle>().IsLeader())
            {
                Ip6::Address aloc;

                node.Get<Mle::Mle>().GetLeaderAloc(aloc);
                jsonWriter.WriteNameValue("leader_aloc", aloc.ToString().AsCString());
                break;
            }
        }
    }

    jsonWriter.BeginObject("topology");

    for (Node &node : mNodes)
    {
        jsonWriter.BeginObject(node.GetId());
        jsonWriter.WriteNameValue("name", node.GetName());
        jsonWriter.WriteNameValue("version", kThreadVersionStringShort);
        jsonWriter.EndObject();
    }
    jsonWriter.EndObject();

    jsonWriter.BeginObject("extaddrs");

    for (Node &node : mNodes)
    {
        jsonWriter.WriteNameValue(node.GetId(), node.Get<Mac::Mac>().GetExtAddress().ToString().AsCString());
    }

    jsonWriter.EndObject();

    jsonWriter.BeginObject("rloc16s");

    for (Node &node : mNodes)
    {
        constexpr uint16_t kRlocStringSize = 10;

        String<kRlocStringSize> rlocString;

        rlocString.Append("0x%04x", node.Get<Mle::Mle>().GetRloc16());
        jsonWriter.WriteNameValue(node.GetId(), rlocString.AsCString());
    }

    jsonWriter.EndObject();

    jsonWriter.BeginObject("mleids");

    for (Node &node : mNodes)
    {
        jsonWriter.WriteNameValue(node.GetId(), node.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    }

    jsonWriter.EndObject();

    jsonWriter.BeginObject("rlocs");

    for (Node &node : mNodes)
    {
        jsonWriter.WriteNameValue(node.GetId(), node.Get<Mle::Mle>().GetMeshLocalRloc().ToString().AsCString());
    }

    jsonWriter.EndObject();

    jsonWriter.BeginObject("ipaddrs");

    for (Node &node : mNodes)
    {
        jsonWriter.BeginArray(node.GetId());

        for (const Ip6::Netif::UnicastAddress &addr : node.Get<ThreadNetif>().GetUnicastAddresses())
        {
            jsonWriter.WriteValue(addr.GetAddress().ToString().AsCString());
        }

        jsonWriter.EndArray();
    }

    jsonWriter.EndObject();

    jsonWriter.BeginObject("extra_vars");

    if (!mNodes.IsEmpty())
    {
        Ip6::Prefix prefix;

        prefix.Set(mNodes.GetHead()->Get<Mle::Mle>().GetMeshLocalPrefix());
        jsonWriter.WriteNameValue("mesh_local_prefix", prefix.ToString().AsCString());
    }

    jsonWriter.EndObject();

    jsonWriter.CloseFile();

exit:
    return;
}

Core::~Core(void) { sInUse = false; }

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
    ProcessMdns(aNode);
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    ProcessTrel(aNode);
#endif

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

    mPcap.WriteFrame(aNode.mRadio.mTxFrame, mNow.GetValue() * 1000ull);

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
        Radio::Frame ackFrame;

        ackFrame.GenerateImmAck(
            static_cast<const Mac::RxFrame &>(static_cast<const Mac::Frame &>(aNode.mRadio.mTxFrame)),
            (ackMode == kSendAckFramePending));

        ackFrame.UpdateFcs();
        mPcap.WriteFrame(ackFrame, mNow.GetValue() * 1000ull);

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

void Core::ProcessMdns(Node &aNode)
{
    Mdns::AddressInfo senderAddress;

    aNode.mMdns.GetAddress(senderAddress);

    for (Mdns::PendingTx &pendingTx : aNode.mMdns.mPendingTxList)
    {
        for (Node &rxNode : mNodes)
        {
            rxNode.mMdns.Receive(rxNode.GetInstance(), pendingTx, senderAddress);
        }
    }

    aNode.mMdns.mPendingTxList.Free();
}

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

void Core::ProcessTrel(Node &aNode)
{
    Ip6::SockAddr senderSockAddr;
    Ip6::SockAddr rxNodeSockAddr;

    aNode.GetTrelSockAddr(senderSockAddr);

    for (Trel::PendingTx &pendingTx : aNode.mTrel.mPendingTxList)
    {
        for (Node &rxNode : mNodes)
        {
            rxNode.GetTrelSockAddr(rxNodeSockAddr);

            if (pendingTx.mDestSockAddr == rxNodeSockAddr)
            {
                rxNode.mTrel.Receive(rxNode.GetInstance(), pendingTx.mPayloadData, senderSockAddr);
                break;
            }
        }
    }

    aNode.mTrel.mPendingTxList.Free();
}

#endif // OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

//---------------------------------------------------------------------------------------------------------------------

Core::IcmpEchoResponseContext::IcmpEchoResponseContext(Node &aNode, uint16_t aIdentifier)
    : mNode(aNode)
    , mIdentifier(aIdentifier)
    , mResponseReceived(false)
{
}

void Core::HandleIcmpResponse(void                *aContext,
                              otMessage           *aMessage,
                              const otMessageInfo *aMessageInfo,
                              const otIcmp6Header *aIcmpHeader)
{
    OT_UNUSED_VARIABLE(aMessage);

    IcmpEchoResponseContext *context     = static_cast<IcmpEchoResponseContext *>(aContext);
    const Ip6::Icmp::Header *header      = AsCoreTypePtr(aIcmpHeader);
    const Ip6::MessageInfo  *messageInfo = AsCoreTypePtr(aMessageInfo);

    VerifyOrQuit(context != nullptr);
    VerifyOrQuit(header != nullptr);
    VerifyOrQuit(messageInfo != nullptr);

    if ((header->GetType() == Ip6::Icmp::Header::kTypeEchoReply) && (header->GetId() == context->mIdentifier))
    {
        context->mResponseReceived = true;

        Log("Received Echo Reply on Node %u (%s) from %s", context->mNode.GetId(), context->mNode.GetName(),
            messageInfo->GetPeerAddr().ToString().AsCString());
    }
}

void Core::SendAndVerifyEchoRequest(Node               &aSender,
                                    const Ip6::Address &aDestination,
                                    uint16_t            aPayloadSize,
                                    uint8_t             aHopLimit,
                                    uint32_t            aResponseTimeout)
{
    static constexpr uint16_t kIdentifier = 0x1234;

    IcmpEchoResponseContext icmpContext(aSender, kIdentifier);
    Ip6::Icmp::Handler      icmpHandler(HandleIcmpResponse, &icmpContext);

    SuccessOrQuit(aSender.Get<Ip6::Icmp>().RegisterHandler(icmpHandler));

    aSender.SendEchoRequest(aDestination, kIdentifier, aPayloadSize, aHopLimit);
    AdvanceTime(aResponseTimeout);
    VerifyOrQuit(icmpContext.mResponseReceived);

    SuccessOrQuit(aSender.Get<Ip6::Icmp>().UnregisterHandler(icmpHandler));
}

} // namespace Nexus
} // namespace ot
