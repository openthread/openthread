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
#include "nexus_sim.hpp"

#include <cstdio>
#include <cstdlib>

#include "mac_frame.h"
#include "nexus_node.hpp"
#include "nexus_radio_model.hpp"
#include "thread/child_table.hpp"
#include "thread/mle.hpp"

namespace ot {
namespace Nexus {

Core *Core::sCore  = nullptr;
bool  Core::sInUse = false;

Core::Core(void)
    : mCurNodeId(0)
    , mPendingAction(false)
    , mSaveNodeLogs(false)
    , mNow(0)
{
    const char *pcapFile;
    const char *saveLogs;

    VerifyOrQuit(!sInUse);
    sCore  = this;
    sInUse = true;

    mNextAlarmTime = NumericLimits<uint64_t>::kMax;

    pcapFile = getenv("OT_NEXUS_PCAP_FILE");

    if ((pcapFile != nullptr) && (pcapFile[0] != '\0'))
    {
        mPcap.Open(pcapFile);
    }

    saveLogs = getenv("OT_NEXUS_SAVE_LOGS");

    if (saveLogs != nullptr)
    {
        static const char *kActivateStrings[] = {"1", "yes", "y", "true", "t", "on"};

        bool activate = false;

        for (const char *activateString : kActivateStrings)
        {
            if (StringMatch(saveLogs, activateString, kStringCaseInsensitiveMatch))
            {
                activate = true;
                break;
            }
        }

        mSaveNodeLogs = activate;
    }
}

void Core::SaveTestInfo(const char *aFilename, Node *aLeaderNode)
{
    FILE       *file = fopen(aFilename, "w");
    Node       *tail = mNodes.GetTail();
    const char *testcase;
    const char *slash;
    const char *dot;
    const char *version;
    int         testcaseLen;
    Node       *leaderNode = aLeaderNode;

    VerifyOrExit(file != nullptr);

    testcase = aFilename;
    slash    = strrchr(aFilename, '/');

    if (slash != nullptr)
    {
        testcase = slash + 1;
    }

    dot         = strrchr(testcase, '.');
    testcaseLen = (dot != nullptr) ? static_cast<int>(dot - testcase) : static_cast<int>(strlen(testcase));

    switch (otThreadGetVersion())
    {
    case OT_THREAD_VERSION_1_1:
        version = "1.1";
        break;
    case OT_THREAD_VERSION_1_2:
        version = "1.2";
        break;
    case OT_THREAD_VERSION_1_3:
        version = "1.3";
        break;
    case OT_THREAD_VERSION_1_4:
        version = "1.4";
        break;
    default:
        version = "unknown";
        break;
    }

    fprintf(file, "{\n");
    fprintf(file, "  \"testcase\": \"%.*s\",\n", testcaseLen, testcase);
    fprintf(file, "  \"pcap\": \"%s\",\n", getenv("OT_NEXUS_PCAP_FILE") ? getenv("OT_NEXUS_PCAP_FILE") : "");

    if (leaderNode == nullptr)
    {
        for (Node &node : mNodes)
        {
            if (node.Get<Mle::Mle>().IsLeader())
            {
                leaderNode = &node;
                break;
            }
        }
    }

    if (leaderNode == nullptr)
    {
        leaderNode = mNodes.GetTail();
    }

    if (leaderNode != nullptr)
    {
        NetworkKey                          networkKey;
        String<OT_NETWORK_KEY_SIZE * 2 + 1> keyString;

        leaderNode->Get<KeyManager>().GetNetworkKey(networkKey);
        keyString.AppendHexBytes(networkKey.m8, OT_NETWORK_KEY_SIZE);
        fprintf(file, "  \"network_key\": \"%s\",\n", keyString.AsCString());

        fprintf(file, "  \"network_keys\": [\n");
        for (const NetworkKey &key : mNetworkKeys)
        {
            String<OT_NETWORK_KEY_SIZE * 2 + 1> keyStr;
            keyStr.AppendHexBytes(key.m8, OT_NETWORK_KEY_SIZE);
            fprintf(file, "    \"%s\"%s\n", keyStr.AsCString(), (&key == mNetworkKeys.Back()) ? "" : ",");
        }
        fprintf(file, "  ],\n");

        if (leaderNode->Get<Mle::Mle>().IsLeader())
        {
            Ip6::Address aloc;
            leaderNode->Get<Mle::Mle>().GetLeaderAloc(aloc);
            fprintf(file, "  \"leader_aloc\": \"%s\",\n", aloc.ToString().AsCString());
        }
    }

    fprintf(file, "  \"topology\": {\n");
    for (Node &node : mNodes)
    {
        fprintf(file, "    \"%u\": {\"name\": \"%s\", \"version\": \"%s\"}%s\n", node.GetInstance().GetId(),
                node.GetName() ? node.GetName() : "", version, (&node == tail) ? "" : ",");
    }
    fprintf(file, "  },\n");

    fprintf(file, "  \"extaddrs\": {\n");
    for (Node &node : mNodes)
    {
        fprintf(file, "    \"%u\": \"%s\"%s\n", node.GetInstance().GetId(),
                node.Get<Mac::Mac>().GetExtAddress().ToString().AsCString(), (&node == tail) ? "" : ",");
    }
    fprintf(file, "  },\n");

    fprintf(file, "  \"ipaddrs\": {\n");
    for (Node &node : mNodes)
    {
        bool first = true;

        fprintf(file, "    \"%u\": [\n", node.GetInstance().GetId());

        for (const Ip6::Netif::UnicastAddress &addr : node.Get<Ip6::Netif>().GetUnicastAddresses())
        {
            fprintf(file, "%s      \"%s\"", first ? "" : ",\n", addr.GetAddress().ToString().AsCString());
            first = false;
        }

        for (const Ip6::Address &infraAddr : node.mInfraIf.GetAddresses())
        {
            fprintf(file, "%s      \"%s\"", first ? "" : ",\n", infraAddr.ToString().AsCString());
            first = false;
        }

        fprintf(file, "\n    ]%s\n", (&node == tail) ? "" : ",");
    }
    fprintf(file, "  },\n");

    fprintf(file, "  \"ethaddrs\": {\n");
    for (Node &node : mNodes)
    {
        InfraIf::LinkLayerAddress mac;

        node.mInfraIf.GetLinkLayerAddress(mac);
        fprintf(file, "    \"%u\": \"%s\"%s\n", node.GetInstance().GetId(), mac.ToString().AsCString(),
                (&node == tail) ? "" : ",");
    }
    fprintf(file, "  },\n");

    fprintf(file, "  \"rloc16s\": {\n");
    for (Node &node : mNodes)
    {
        fprintf(file, "    \"%u\": \"0x%04x\"%s\n", node.GetInstance().GetId(), node.Get<Mle::Mle>().GetRloc16(),
                (&node == tail) ? "" : ",");
    }
    fprintf(file, "  },\n");

    fprintf(file, "  \"mleids\": {\n");
    for (Node &node : mNodes)
    {
        fprintf(file, "    \"%u\": \"%s\"%s\n", node.GetInstance().GetId(),
                node.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString(), (&node == tail) ? "" : ",");
    }
    fprintf(file, "  },\n");

    fprintf(file, "  \"rlocs\": {\n");
    for (Node &node : mNodes)
    {
        fprintf(file, "    \"%u\": \"%s\"%s\n", node.GetInstance().GetId(),
                node.Get<Mle::Mle>().GetMeshLocalRloc().ToString().AsCString(), (&node == tail) ? "" : ",");
    }
    fprintf(file, "  },\n");

    fprintf(file, "  \"channels\": {\n");
    for (Node &node : mNodes)
    {
        fprintf(file, "    \"%u\": %u%s\n", node.GetInstance().GetId(), node.Get<Mac::Mac>().GetPanChannel(),
                (&node == tail) ? "" : ",");
    }
    fprintf(file, "  },\n");

    fprintf(file, "  \"trel_udp_ports\": {\n");
    for (Node &node : mNodes)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        fprintf(file, "    \"%u\": %u%s\n", node.GetInstance().GetId(), node.mTrel.mUdpPort,
                (&node == tail) ? "" : ",");
#else
        fprintf(file, "    \"%u\": 0%s\n", node.GetInstance().GetId(), (&node == tail) ? "" : ",");
#endif
    }
    fprintf(file, "  },\n");

    fprintf(file, "  \"extra_vars\": {\n");
    if (leaderNode != nullptr)
    {
        Ip6::Prefix prefix;
        prefix.Set(leaderNode->Get<Mle::Mle>().GetMeshLocalPrefix());
        fprintf(file, "    \"mesh_local_prefix\": \"%s\"%s\n", prefix.ToString().AsCString(),
                mTestVars.IsEmpty() ? "" : ",");
    }
    for (const TestVar &var : mTestVars)
    {
        fprintf(file, "    \"%s\": \"%s\"%s\n", var.mName.AsCString(), var.mValue.AsCString(),
                (&var == mTestVars.Back()) ? "" : ",");
    }
    fprintf(file, "  }\n");

    fprintf(file, "}\n");

exit:
    if (file != nullptr)
    {
        fclose(file);
    }
}

void Core::AddNetworkKey(const NetworkKey &aKey) { SuccessOrQuit(mNetworkKeys.PushBack(aKey)); }

Core::TestVar &Core::NewTestVar(const char *aName)
{
    TestVar *var = mTestVars.PushBack();

    VerifyOrQuit(var != nullptr);
    var->mName.Clear().Append("%s", aName);
    var->mValue.Clear();

    return *var;
}

void Core::AddTestVar(const char *aName, const char *aValue) { NewTestVar(aName).mValue.Append("%s", aValue); }

void Core::AddTestVar(const char *aName, uint32_t aUintValue)
{
    NewTestVar(aName).mValue.Append("%lu", ToUlong(aUintValue));
}

void Core::AddOmrPrefixTestVar(const char *aName, Node &aNode)
{
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    BorderRouter::RoutingManager &routingManager = aNode.Get<BorderRouter::RoutingManager>();
    Ip6::Prefix                   omrPrefix;
    BorderRouter::RoutePreference preference;
    String<17>                    omrPrefixString;

    if (routingManager.GetFavoredOmrPrefix(omrPrefix, preference) != kErrorNone)
    {
        SuccessOrQuit(routingManager.GetOmrPrefix(omrPrefix));
    }

    omrPrefixString.AppendHexBytes(omrPrefix.GetBytes(), 8);
    AddTestVar(aName, omrPrefixString.AsCString());
#else
    OT_UNUSED_VARIABLE(aName);
    OT_UNUSED_VARIABLE(aNode);
#endif
}

Core::~Core(void)
{
    while (!mNodes.IsEmpty())
    {
        Node *node = mNodes.GetHead();

        UpdateActiveInstance(&node->GetInstance());
        mNodes.Pop();
    }

    UpdateActiveInstance(nullptr);
    sInUse = false;
}

void Core::NotifyHeartbeat(void)
{
    for (Observer &observer : mObservers)
    {
        observer.OnHeartbeat(mNow);
    }
}

void Core::NotifyDumpState(void)
{
    for (Observer &observer : mObservers)
    {
        observer.DumpState();
    }
}

void Core::SetNodeEnabled(uint32_t aNodeId, bool aEnabled)
{
    Node *node = FindNodeById(aNodeId);

    if (node != nullptr)
    {
        Log("Core::SetNodeEnabled: Node %u found. Setting Thread enabled=%d", aNodeId, aEnabled);

        if (!aEnabled)
        {
            node->Get<ThreadNetif>().Down();
            node->Get<Mle::Mle>().Stop();
        }
        else
        {
            node->Get<ThreadNetif>().Up();
            SuccessOrQuit(node->Get<Mle::Mle>().Start());
        }
    }
}

Node *Core::FindNodeById(uint32_t aNodeId) { return mNodes.FindMatching(aNodeId); }

Node *Core::FindNodeByExtAddress(const Mac::ExtAddress &aExtAddress) { return mNodes.FindMatching(aExtAddress); }

Node &Core::CreateNode(void)
{
    Node *node;

    node = Node::Allocate();
    VerifyOrQuit(node != nullptr);

    node->GetInstance().SetId(mCurNodeId++);

    if (mSaveNodeLogs)
    {
        node->mLogging.Init(node->GetId());
    }

    node->mInfraIf.AfterInit();

    node->mMdns.Init(*node);
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    node->mTrel.Init(*node);
#endif

    mNodes.Push(*node);

    node->GetInstance().AfterInit();

    node->Get<Ip6::Ip6>().SetReceiveCallback(Node::HandleIp6Receive, node);

    node->Get<NeighborTable>().RegisterCallback(&Core::HandleNeighborTableChanged);
    SuccessOrQuit(node->Get<Notifier>().RegisterCallback(&Core::HandleStateChanged, node));

    for (Observer &observer : mObservers)
    {
        observer.OnNodeStateChanged(node);
    }

    return *node;
}

void Core::HandleNeighborTableChanged(otNeighborTableEvent aEvent, const otNeighborTableEntryInfo *aInfo)
{
    NeighborTable::Event event = static_cast<NeighborTable::Event>(aEvent);

    Core    &core     = Core::Get();
    uint32_t srcId    = 0;
    uint32_t dstId    = 0;
    bool     foundSrc = false;
    bool     foundDst = false;
    bool     isActive;

    const Mac::ExtAddress *extAddr = nullptr;

    VerifyOrExit(!core.mObservers.IsEmpty());

    Log("HandleNeighborTableChanged: Event %d", event);

    {
        Node &srcNode = Node::From(aInfo->mInstance);

        srcId    = srcNode.GetId();
        foundSrc = true;
    }

    switch (event)
    {
    case NeighborTable::kChildAdded:
    case NeighborTable::kChildRemoved:
        extAddr  = &AsCoreType(&aInfo->mInfo.mChild.mExtAddress);
        isActive = (event == NeighborTable::kChildAdded);

        if (event == NeighborTable::kChildRemoved)
        {
            Instance &instance = *static_cast<Instance *>(aInfo->mInstance);

            if (instance.Get<RouterTable>().FindRouter(*extAddr) != nullptr)
            {
                Log("Suppressing CHILD_REMOVED event because neighbor is a valid router");
                ExitNow();
            }
        }
        break;

    case NeighborTable::kRouterAdded:
    case NeighborTable::kRouterRemoved:
        extAddr  = &AsCoreType(&aInfo->mInfo.mRouter.mExtAddress);
        isActive = (event == NeighborTable::kRouterAdded);
        break;

    default:
        break;
    }

    if (extAddr != nullptr)
    {
        Node *node = core.GetNodes().FindMatching(*extAddr);

        if (node != nullptr)
        {
            dstId    = node->GetInstance().GetId();
            foundDst = true;
        }
    }

    if (foundSrc && foundDst)
    {
        for (Observer &observer : core.mObservers)
        {
            observer.OnLinkUpdate(srcId, dstId, isActive);
        }
    }
    else
    {
        Log("HandleNeighborTableChanged: Failed to find srcId or dstId. foundSrc: %d, foundDst: %d", foundSrc,
            foundDst);
    }

exit:
    return;
}

void Core::HandleStateChanged(otChangedFlags aFlags, void *aContext)
{
    OT_UNUSED_VARIABLE(aFlags);

    Core &core = Core::Get();
    Node *node = static_cast<Node *>(aContext);

    VerifyOrExit(!core.mObservers.IsEmpty() && node != nullptr);

    for (Observer &observer : core.mObservers)
    {
        observer.OnNodeStateChanged(node);
    }

    // Decoupled from flags to capture SED parent changes
    switch (node->Get<Mle::Mle>().GetRole())
    {
    case Mle::kRoleChild:
    {
        Router::Info parentInfo;

        if (node->Get<Mle::Mle>().GetParentInfo(parentInfo) == kErrorNone)
        {
            uint32_t srcId  = node->GetInstance().GetId();
            uint32_t dstId  = 0xffff;
            Node    *rxNode = core.mNodes.FindMatching(AsCoreType(&parentInfo.mExtAddress));

            if (rxNode != nullptr)
            {
                dstId = rxNode->GetInstance().GetId();
            }

            if (dstId != 0xffff && dstId != node->GetLastParentId())
            {
                if (node->GetLastParentId() != 0xffff)
                {
                    for (Observer &observer : core.mObservers)
                    {
                        observer.OnLinkUpdate(srcId, node->GetLastParentId(), false);
                    }
                }
                node->SetLastParentId(dstId);
                for (Observer &observer : core.mObservers)
                {
                    observer.OnLinkUpdate(srcId, dstId, true);
                }
            }
        }
        break;
    }
    case Mle::kRoleDetached:
    {
        uint32_t srcId = node->GetInstance().GetId();

        for (Node &rxNode : core.mNodes)
        {
            if (&rxNode == node)
            {
                continue;
            }

            if (node->Get<NeighborTable>().FindNeighbor(rxNode.Get<Mac::Mac>().GetExtAddress(),
                                                        Neighbor::kInStateValid) != nullptr)
            {
                uint32_t dstId = rxNode.GetInstance().GetId();

                for (Observer &observer : core.mObservers)
                {
                    observer.OnLinkUpdate(srcId, dstId, false);
                    observer.OnLinkUpdate(dstId, srcId, false);
                }
            }
        }

        if (node->GetLastParentId() != 0xffff)
        {
            for (Observer &observer : core.mObservers)
            {
                observer.OnLinkUpdate(srcId, node->GetLastParentId(), false);
            }
            node->SetLastParentId(0xffff);
        }
        break;
    }
    case Mle::kRoleRouter:
    case Mle::kRoleLeader:
        node->SetLastParentId(0xffff);
        break;

    default:
        break;
    }

exit:
    return;
}

void Core::UpdateNextAlarmMilli(const Alarm &aAlarm)
{
    if (aAlarm.mScheduled)
    {
        uint64_t alarmTime;

        if (GetNow() >= aAlarm.mAlarmTime)
        {
            alarmTime = mNow;
        }
        else
        {
            alarmTime = mNow - (mNow % 1000u) + (static_cast<uint64_t>(aAlarm.mAlarmTime - GetNow()) * 1000u);
        }

        mNextAlarmTime = Min(mNextAlarmTime, alarmTime);
    }
}

void Core::UpdateNextAlarmMicro(const Alarm &aAlarm)
{
    if (aAlarm.mScheduled)
    {
        uint64_t alarmTime;

        if (GetNowMicro() >= aAlarm.mAlarmTime)
        {
            alarmTime = mNow;
        }
        else
        {
            alarmTime = mNow + static_cast<uint64_t>(aAlarm.mAlarmTime - GetNowMicro());
        }

        mNextAlarmTime = Min(mNextAlarmTime, alarmTime);
    }
}

bool Core::IsUiConnected(void) const
{
    bool connected = false;

    for (const Observer &observer : mObservers)
    {
        if (observer.IsConnected())
        {
            connected = true;
            break;
        }
    }

    return connected;
}

void Core::Reset(void)
{
    mNodes.Clear();
    mCurNodeId     = 0;
    mNow           = 0;
    mNextAlarmTime = NumericLimits<uint64_t>::kMax;

    for (Observer &observer : mObservers)
    {
        observer.OnClearEvents();
    }
}

void Core::AdvanceTime(uint32_t aDuration)
{
    uint64_t targetTime = mNow + (static_cast<uint64_t>(aDuration) * 1000u);

    while (mPendingAction || (mNextAlarmTime <= targetTime))
    {
        mNextAlarmTime = NumericLimits<uint64_t>::kMax;
        mPendingAction = false;

        for (Node &node : mNodes)
        {
            Process(node);
            UpdateNextAlarmMilli(node.mAlarmMilli);
            UpdateNextAlarmMicro(node.mAlarmMicro);
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
    ProcessInfraIf(aNode);

    if (aNode.mAlarmMilli.mScheduled && (GetNow() >= aNode.mAlarmMilli.mAlarmTime))
    {
        aNode.mAlarmMilli.mScheduled = false;
        otPlatAlarmMilliFired(&aNode.GetInstance());
    }

    if (aNode.mAlarmMicro.mScheduled && (GetNowMicro() >= aNode.mAlarmMicro.mAlarmTime))
    {
        aNode.mAlarmMicro.mScheduled = false;
        otPlatAlarmMicroFired(&aNode.GetInstance());
    }
}

void Core::ProcessRadio(Node &aNode)
{
    Mac::Address dstAddr;
    uint16_t     dstPanId;
    bool         ackRequested;
    AckMode      ackMode = kNoAck;
    Node        *ackNode = nullptr;

    VerifyOrExit(aNode.mRadio.mState == Radio::kStateTransmit);

    if (aNode.mRadio.mTxFrame.GetDstAddr(dstAddr) != kErrorNone)
    {
        dstAddr.SetNone();
    }

    if (aNode.mRadio.mTxFrame.GetDstPanId(dstPanId) != kErrorNone)
    {
        dstPanId = Mac::kPanIdBroadcast;
    }

    ackRequested                           = aNode.mRadio.mTxFrame.GetAckRequest();
    aNode.mRadio.mRadioContext.mCslPresent = aNode.mRadio.mTxFrame.mInfo.mTxInfo.mCslPresent;

    SuccessOrQuit(otMacFrameProcessTxSfd(&aNode.mRadio.mTxFrame, mNow, &aNode.mRadio.mRadioContext));
    static_cast<Radio::Frame &>(aNode.mRadio.mTxFrame).UpdateFcs();

    mPcap.WriteFrame(aNode.mRadio.mTxFrame, mNow);

    if (!mObservers.IsEmpty())
    {
        uint32_t dstNodeId = 0xffff; // Default to broadcast / unknown

        if (!dstAddr.IsBroadcast())
        {
            for (Node &rxNode : mNodes)
            {
                if (&rxNode == &aNode)
                {
                    continue;
                }

                if (rxNode.mRadio.Matches(dstAddr, dstPanId))
                {
                    dstNodeId = rxNode.GetInstance().GetId();
                    break;
                }
            }
        }

        if (!dstAddr.IsBroadcast() && dstNodeId == 0xffff)
        {
            Log("ProcessRadio: Failed to resolve dstNodeId for unicast from node %u to %s", aNode.GetInstance().GetId(),
                dstAddr.ToString().AsCString());
        }

        for (Observer &observer : mObservers)
        {
            observer.OnPacketEvent(aNode.GetInstance().GetId(), dstNodeId, aNode.mRadio.mTxFrame.GetPsdu(),
                                   aNode.mRadio.mTxFrame.GetLength());
        }
    }

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

            rxFrame.mInfo.mRxInfo.mTimestamp = mNow;

            int16_t localRssi = RadioModel::CalculateRssi(aNode, rxNode);

            // Completely intercept and drop packets that dip below target receiver sensitivity
            if (RadioModel::ShouldDropPacket(localRssi))
            {
                continue;
            }

            rxFrame.mInfo.mRxInfo.mRssi = ClampToInt8(localRssi);

            rxFrame.mInfo.mRxInfo.mLqi = kDefaultRxLqi;

            if (matchesDst && !dstAddr.IsNone() && !dstAddr.IsBroadcast() && ackRequested)
            {
                Mac::Address srcAddr;

                ackMode = kSendAckNoFramePending;
                ackNode = &rxNode;

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

    if (ackNode != nullptr)
    {
        Radio::Frame        ackFrame;
        const Mac::RxFrame &rxFrame =
            static_cast<const Mac::RxFrame &>(static_cast<const Mac::Frame &>(aNode.mRadio.mTxFrame));

        if (rxFrame.IsVersion2015())
        {
            uint8_t ackIeData[OT_ACK_IE_MAX_SIZE];
            uint8_t ackIeDataLength = 0;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
            ackNode->mRadio.mRadioContext.mCslPresent =
                (ackNode->mRadio.mRadioContext.mCslPeriod > 0) &&
                otMacFrameSrcAddrMatchCslReceiverPeer(&aNode.mRadio.mTxFrame, &ackNode->mRadio.mRadioContext);

            if (ackNode->mRadio.mRadioContext.mCslPresent)
            {
                ackIeDataLength = otMacFrameGenerateCslIeTemplate(ackIeData);
            }
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
            {
                uint8_t      linkMetricsData[OT_ENH_PROBING_IE_DATA_MAX_SIZE];
                uint8_t      linkMetricsDataLen;
                Mac::Address srcAddr;

                if (aNode.mRadio.mTxFrame.GetSrcAddr(srcAddr) == kErrorNone)
                {
                    linkMetricsDataLen = ackNode->mRadio.GenerateEnhAckProbingData(srcAddr, kDefaultRxLqi,
                                                                                   kDefaultRxRssi, linkMetricsData);

                    if (linkMetricsDataLen > 0)
                    {
                        ackIeDataLength += otMacFrameGenerateEnhAckProbingIe(ackIeData + ackIeDataLength,
                                                                             linkMetricsData, linkMetricsDataLen);
                    }
                }
            }
#endif
            SuccessOrExit(
                ackFrame.GenerateEnhAck(rxFrame, (ackMode == kSendAckFramePending), ackIeData, ackIeDataLength));
            SuccessOrExit(otMacFrameProcessTxSfd(&ackFrame, mNow, &ackNode->mRadio.mRadioContext));
        }
        else
        {
            ackFrame.GenerateImmAck(rxFrame, (ackMode == kSendAckFramePending));
        }

        ackFrame.UpdateFcs();

        {
            int16_t ackRssi = RadioModel::CalculateRssi(*ackNode, aNode);

            ackFrame.mInfo.mRxInfo.mRssi      = ClampToInt8(ackRssi);
            ackFrame.mInfo.mRxInfo.mLqi       = kDefaultRxLqi;
            ackFrame.mInfo.mRxInfo.mTimestamp = mNow;

            mPcap.WriteFrame(ackFrame, mNow);

            if (RadioModel::ShouldDropPacket(ackRssi))
            {
                otPlatRadioTxDone(&aNode.GetInstance(), &aNode.mRadio.mTxFrame, nullptr, kErrorNoAck);
            }
            else
            {
                otPlatRadioTxDone(&aNode.GetInstance(), &aNode.mRadio.mTxFrame, &ackFrame, kErrorNone);
            }
        }
    }
    else
    {
        otPlatRadioTxDone(&aNode.GetInstance(), &aNode.mRadio.mTxFrame, nullptr,
                          ackRequested ? kErrorNoAck : kErrorNone);
    }

exit:
    return;
}

void Core::ProcessInfraIf(Node &aNode)
{
    // Deliver pending packets on the infrastructure interface.

    Message *message;

    while ((message = aNode.mInfraIf.mPendingTxQueue.GetHead()) != nullptr)
    {
        Ip6::Header header;
        Node       *targetNode = nullptr;
        Heap::Data  msgData;

        aNode.mInfraIf.mPendingTxQueue.Dequeue(*message);

        SuccessOrQuit(message->Read(0, header));

        VerifyOrQuit(message->GetLength() >= sizeof(Ip6::Header) && header.IsVersion6());

        SuccessOrQuit(msgData.SetFrom(*message, 0, message->GetLength()));

        {
            InfraIf::LinkLayerAddress srcMac;
            InfraIf::LinkLayerAddress dstMac;

            aNode.mInfraIf.GetLinkLayerAddress(srcMac);

            if (header.GetDestination().IsMulticast())
            {
                dstMac.mLength     = 6;
                dstMac.mAddress[0] = 0x33;
                dstMac.mAddress[1] = 0x33;
                dstMac.mAddress[2] = header.GetDestination().mFields.m8[12];
                dstMac.mAddress[3] = header.GetDestination().mFields.m8[13];
                dstMac.mAddress[4] = header.GetDestination().mFields.m8[14];
                dstMac.mAddress[5] = header.GetDestination().mFields.m8[15];
            }
            else
            {
                Node *dstNode = FindNodeByInfraIfAddress(header.GetDestination());

                if (dstNode != nullptr)
                {
                    dstNode->mInfraIf.GetLinkLayerAddress(dstMac);
                }
                else
                {
                    dstMac.mLength = 6;
                    memset(dstMac.mAddress, 0xff, 6);
                }
            }

            mPcap.WritePacket(srcMac, dstMac, msgData.GetBytes(), msgData.GetLength(), mNow);
        }

        if (!header.GetDestination().IsMulticast())
        {
            if (!IsThreadAddress(header.GetDestination()))
            {
                targetNode = FindNodeByAddress(header.GetDestination());
            }
        }

        for (Node &rxNode : mNodes)
        {
            if (&rxNode == &aNode)
            {
                continue;
            }

            if (targetNode != nullptr && &rxNode != targetNode)
            {
                continue;
            }

            rxNode.mInfraIf.Receive(*message);
        }

        message->Free();
    }
}

Node *Core::FindNodeByAddress(const Ip6::Address &aAddress)
{
    return mNodes.FindMatching(aAddress, Node::kAnyNetifAddress);
}

bool Core::IsThreadAddress(const Ip6::Address &aAddress)
{
    return mNodes.ContainsMatching(aAddress, Node::kThreadNetifAddress);
}

Node *Core::FindNodeByThreadAddress(const Ip6::Address &aAddress)
{
    return mNodes.FindMatching(aAddress, Node::kThreadNetifAddress);
}

Node *Core::FindNodeByInfraIfAddress(const Ip6::Address &aAddress)
{
    return mNodes.FindMatching(aAddress, Node::kInfraNetifAddress);
}

//---------------------------------------------------------------------------------------------------------------------

Core::IcmpEchoResponseContext::IcmpEchoResponseContext(Node &aNode, uint16_t aIdentifier)
    : mNode(aNode)
    , mIdentifier(aIdentifier)
    , mResponseReceived(false)
    , mExpectedSourceCheck(false)
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

        if (context->mExpectedSourceCheck)
        {
            Log("Verifying source address: Expected %s, Actual %s", context->mExpectedSource.ToString().AsCString(),
                messageInfo->GetSockAddr().ToString().AsCString());

            VerifyOrQuit(messageInfo->GetSockAddr() == context->mExpectedSource);
        }
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

    SuccessOrQuit(aSender.Get<Ip6::Icmp>().UnregisterHandler(icmpHandler));

    VerifyOrQuit(icmpContext.mResponseReceived);
}

void Core::SendAndVerifyEchoRequest(Node               &aSender,
                                    const Ip6::Address &aExpectedSource,
                                    const Ip6::Address &aDestination,
                                    uint16_t            aPayloadSize,
                                    uint8_t             aHopLimit,
                                    uint32_t            aResponseTimeout,
                                    bool                aForceSource)
{
    static constexpr uint16_t kIdentifier = 0x1234;

    IcmpEchoResponseContext icmpContext(aSender, kIdentifier);
    icmpContext.mExpectedSource      = aExpectedSource;
    icmpContext.mExpectedSourceCheck = true;

    Ip6::Icmp::Handler icmpHandler(HandleIcmpResponse, &icmpContext);

    SuccessOrQuit(aSender.Get<Ip6::Icmp>().RegisterHandler(icmpHandler));

    aSender.SendEchoRequest(aDestination, kIdentifier, aPayloadSize, aHopLimit,
                            aForceSource ? &aExpectedSource : nullptr);
    AdvanceTime(aResponseTimeout);

    SuccessOrQuit(aSender.Get<Ip6::Icmp>().UnregisterHandler(icmpHandler));

    VerifyOrQuit(icmpContext.mResponseReceived);
}

void Core::SendAndVerifyNoEchoResponse(Node               &aSender,
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

    SuccessOrQuit(aSender.Get<Ip6::Icmp>().UnregisterHandler(icmpHandler));

    VerifyOrQuit(!icmpContext.mResponseReceived);
}

void Core::SendAndVerifyNoEchoResponse(Node               &aSender,
                                       const Ip6::Address &aSrcAddress,
                                       const Ip6::Address &aDestination,
                                       uint16_t            aPayloadSize,
                                       uint8_t             aHopLimit,
                                       uint32_t            aResponseTimeout)
{
    static constexpr uint16_t kIdentifier = 0x1234;

    IcmpEchoResponseContext icmpContext(aSender, kIdentifier);
    Ip6::Icmp::Handler      icmpHandler(HandleIcmpResponse, &icmpContext);

    SuccessOrQuit(aSender.Get<Ip6::Icmp>().RegisterHandler(icmpHandler));

    aSender.SendEchoRequest(aDestination, kIdentifier, aPayloadSize, aHopLimit, &aSrcAddress);
    AdvanceTime(aResponseTimeout);

    SuccessOrQuit(aSender.Get<Ip6::Icmp>().UnregisterHandler(icmpHandler));

    VerifyOrQuit(!icmpContext.mResponseReceived);
}

} // namespace Nexus
} // namespace ot
