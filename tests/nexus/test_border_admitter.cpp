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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

using Admitter     = MeshCoP::BorderAgent::Admitter;
using Manager      = MeshCoP::BorderAgent::Manager;
using Commissioner = MeshCoP::Commissioner;
using Joiner       = MeshCoP::Joiner;

static LogLevel kLogLevel = kLogLevelCrit;

void TestBorderAdmitterPrimeSelection(void)
{
    Core                           nexus;
    Node                          &leader  = nexus.CreateNode();
    Node                          &node1   = nexus.CreateNode();
    Node                          &node2   = nexus.CreateNode();
    Node                          &node3   = nexus.CreateNode();
    Node                          *nodes[] = {&node1, &node2, &node3};
    NetworkData::Service::Iterator netDataIter(leader.GetInstance());
    uint16_t                       rloc16;
    bool                           found;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestBorderAdmitterPrimeSelection");

    nexus.AdvanceTime(0);

    leader.GetInstance().SetLogLevel(kLogLevel);

    // Form the topology.

    leader.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);
    node1.Join(leader);
    node2.Join(leader);
    node3.Join(leader);

    nexus.AdvanceTime(10 * Time::kOneMinuteInMsec);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(node1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(node2.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(node3.Get<Mle::Mle>().IsRouter());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check Border Admitter initial state");

    VerifyOrQuit(!node1.Get<Admitter>().IsEnabled());
    VerifyOrQuit(!node2.Get<Admitter>().IsEnabled());
    VerifyOrQuit(!node3.Get<Admitter>().IsEnabled());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Enable Admitter role on `node1` and validate that it becomes the Prime Admitter");

    node1.Get<Admitter>().SetEnabled(true);
    VerifyOrQuit(node1.Get<Admitter>().IsEnabled());

    nexus.AdvanceTime(45 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(!node1.Get<Admitter>().IsActiveCommissioner());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that the NetworkData contains a single Admitter Service from `node1`");

    netDataIter.Reset();

    SuccessOrQuit(netDataIter.GetNextBorderAdmitterInfo(rloc16));
    VerifyOrQuit(rloc16 == node1.Get<Mle::Mle>().GetRloc16());

    VerifyOrQuit(netDataIter.GetNextBorderAdmitterInfo(rloc16) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Enable Admitter role on `node2` & `node3` and validate that `node1` remains the Prime Admitter");

    node2.Get<Admitter>().SetEnabled(true);
    VerifyOrQuit(node2.Get<Admitter>().IsEnabled());

    node3.Get<Admitter>().SetEnabled(true);
    VerifyOrQuit(node3.Get<Admitter>().IsEnabled());

    nexus.AdvanceTime(45 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(!node1.Get<Admitter>().IsActiveCommissioner());

    VerifyOrQuit(!node2.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(!node3.Get<Admitter>().IsPrimeAdmitter());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that the NetworkData contains the Admitter Service from `node1`");

    netDataIter.Reset();

    SuccessOrQuit(netDataIter.GetNextBorderAdmitterInfo(rloc16));
    VerifyOrQuit(rloc16 == node1.Get<Mle::Mle>().GetRloc16());

    VerifyOrQuit(netDataIter.GetNextBorderAdmitterInfo(rloc16) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disable Admitter role on `node1` and check that another Prime Admitter is elected");

    node1.Get<Admitter>().SetEnabled(false);
    VerifyOrQuit(!node1.Get<Admitter>().IsEnabled());
    VerifyOrQuit(!node1.Get<Admitter>().IsPrimeAdmitter());

    nexus.AdvanceTime(75 * Time::kOneSecondInMsec);

    // We use `!=` to do an "exclusive or" logic check (either node2 or node3 is prime and not both)
    VerifyOrQuit(node2.Get<Admitter>().IsPrimeAdmitter() != node3.Get<Admitter>().IsPrimeAdmitter());

    VerifyOrQuit(!node2.Get<Admitter>().IsActiveCommissioner());
    VerifyOrQuit(!node3.Get<Admitter>().IsActiveCommissioner());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that the NetworkData contains a single Admitter Service entry");

    netDataIter.Reset();

    SuccessOrQuit(netDataIter.GetNextBorderAdmitterInfo(rloc16));

    VerifyOrQuit(rloc16 == (node2.Get<Admitter>().IsPrimeAdmitter() ? node2.Get<Mle::Mle>().GetRloc16()
                                                                    : node3.Get<Mle::Mle>().GetRloc16()));

    VerifyOrQuit(netDataIter.GetNextBorderAdmitterInfo(rloc16) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disable Admitter role on all nodes (`node2` and `node3`)");

    node2.Get<Admitter>().SetEnabled(false);
    VerifyOrQuit(!node2.Get<Admitter>().IsEnabled());
    VerifyOrQuit(!node2.Get<Admitter>().IsPrimeAdmitter());

    node3.Get<Admitter>().SetEnabled(false);
    VerifyOrQuit(!node3.Get<Admitter>().IsEnabled());
    VerifyOrQuit(!node3.Get<Admitter>().IsPrimeAdmitter());

    nexus.AdvanceTime(5 * Time::kOneSecondInMsec);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that the NetworkData contains no Admitter Service entry");

    netDataIter.Reset();
    VerifyOrQuit(netDataIter.GetNextBorderAdmitterInfo(rloc16) == kErrorNotFound);

    nexus.AdvanceTime(10 * Time::kOneSecondInMsec);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Enable Admitter role on all 3 nodes at the same time");

    node1.Get<Admitter>().SetEnabled(true);
    node2.Get<Admitter>().SetEnabled(true);
    node3.Get<Admitter>().SetEnabled(true);

    VerifyOrQuit(node1.Get<Admitter>().IsEnabled());
    VerifyOrQuit(node2.Get<Admitter>().IsEnabled());
    VerifyOrQuit(node3.Get<Admitter>().IsEnabled());

    nexus.AdvanceTime(75 * Time::kOneSecondInMsec);

    Log("Validate that we end up with a single Prime Admitter");

    found = false;

    for (Node *node : nodes)
    {
        if (node->Get<Admitter>().IsPrimeAdmitter())
        {
            VerifyOrQuit(!found);
            found  = true;
            rloc16 = node->Get<Mle::Mle>().GetRloc16();
        }

        VerifyOrQuit(!node->Get<Admitter>().IsActiveCommissioner());
    }

    VerifyOrQuit(found);
}

//---------------------------------------------------------------------------------------------------------------------

enum AdmitterState : uint8_t
{
    kAdmitterUnavailable   = 0,
    kAdmitterReady         = 1,
    kAdmitterActive        = 2,
    kAdmitterConflictError = 3,
};

const char *AdmitterStateToString(uint8_t aState)
{
    const char *str = "Unknown";

    switch (aState)
    {
    case kAdmitterUnavailable:
        str = "AdmitterUnavailable";
        break;
    case kAdmitterReady:
        str = "AdmitterReady";
        break;
    case kAdmitterActive:
        str = "AdmitterActive";
        break;
    case kAdmitterConflictError:
        str = "AdmitterConflictError";
        break;
    default:
        break;
    }
    return str;
}

//---------------------------------------------------------------------------------------------------------------------

struct AdmitterInfo
{
    // Tracks information about Admitter from received
    // messages on an Enroller.

    void ParseAdmitterInfo(const Coap::Message &aResponse)
    {
        Error error;

        error = Tlv::Find<MeshCoP::AdmitterStateTlv>(aResponse, mAdmitterState);
        VerifyOrQuit(error == kErrorNone || error == kErrorNotFound);
        mHasAdmitterState = (error == kErrorNone);

        error = Tlv::Find<MeshCoP::CommissionerSessionIdTlv>(aResponse, mCommrSessionId);
        VerifyOrQuit(error == kErrorNone || error == kErrorNotFound);
        mHasCommrSessionId = (error == kErrorNone);

        error = Tlv::Find<MeshCoP::JoinerUdpPortTlv>(aResponse, mJoinerUdp);
        VerifyOrQuit(error == kErrorNone || error == kErrorNotFound);
        mHasJoinerUdp = (error == kErrorNone);
    }

    bool     mHasAdmitterState;
    bool     mHasCommrSessionId;
    bool     mHasJoinerUdp;
    uint8_t  mResponseState;
    uint8_t  mAdmitterState;
    uint16_t mCommrSessionId;
    uint16_t mJoinerUdp;
};

struct ResponseContext : public AdmitterInfo, public Clearable<ResponseContext>
{
    // Tracks information in a TMF response received by an enroller.
    // (populated by `HandleResponse()`).

    bool mReceived;
};

void HandleResponse(void *aContext, Coap::Msg *aMsg, Error aResult)
{
    ResponseContext *responseContext;

    VerifyOrQuit(aContext != nullptr);
    VerifyOrQuit(aMsg != nullptr);

    responseContext = static_cast<ResponseContext *>(aContext);

    VerifyOrQuit(!responseContext->mReceived); // Duplicate response
    responseContext->mReceived = true;

    SuccessOrQuit(Tlv::Find<MeshCoP::StateTlv>(aMsg->mMessage, responseContext->mResponseState));

    responseContext->ParseAdmitterInfo(aMsg->mMessage);

    Log("  Received response - %s",
        MeshCoP::StateTlv::StateToString(static_cast<MeshCoP::StateTlv::State>(responseContext->mResponseState)));
}

//---------------------------------------------------------------------------------------------------------------------

struct ReceiveContext
{
    // Tracks information from received TMF on an Enroller send by
    // Admitter.

    static constexpr uint8_t kMaxEntries = 16;

    void Clear(void)
    {
        mStateReports.Clear();
        mRelayRxMsgs.DequeueAndFreeAll();
        mProxyRxMsgs.DequeueAndFreeAll();
    }

    bool    HasReceivedReportState(void) const { return !mStateReports.IsEmpty(); }
    uint8_t GetLastReportedAdmitterState(void) { return mStateReports.Back()->mAdmitterState; }

    Array<AdmitterInfo, kMaxEntries> mStateReports;
    MessageQueue                     mRelayRxMsgs;
    MessageQueue                     mProxyRxMsgs;
};

bool HandleResource(void *aContext, Uri aUri, Coap::Msg &aMsg)
{
    bool                     didHandle = false;
    ReceiveContext          *recvContext;
    AdmitterInfo            *info;
    Message                 *msgClone;
    uint16_t                 joinerPort;
    uint16_t                 joinerRouterRloc;
    Ip6::InterfaceIdentifier joinerIid;

    VerifyOrQuit(aContext != nullptr);
    recvContext = static_cast<ReceiveContext *>(aContext);

    switch (aUri)
    {
    case kUriEnrollerReportState:
        didHandle = true;
        info      = recvContext->mStateReports.PushBack();
        VerifyOrQuit(info != nullptr);
        info->ParseAdmitterInfo(aMsg.mMessage);
        VerifyOrQuit(info->mHasAdmitterState);
        Log("  Received `EnrollerReportState` with state %s", AdmitterStateToString(info->mAdmitterState));
        break;

    case kUriRelayRx:
        SuccessOrQuit(Tlv::Find<MeshCoP::JoinerUdpPortTlv>(aMsg.mMessage, joinerPort));
        SuccessOrQuit(Tlv::Find<MeshCoP::JoinerIidTlv>(aMsg.mMessage, joinerIid));
        Log("  Received `RelayRx` from joiner - port:%u iid:%s", joinerPort, joinerIid.ToString().AsCString());

        msgClone = aMsg.mMessage.Clone();
        VerifyOrQuit(msgClone != nullptr);
        recvContext->mRelayRxMsgs.Enqueue(*msgClone);
        break;

    case kUriProxyRx:
        Log("  Received `ProxyRx`");
        msgClone = aMsg.mMessage.Clone();
        VerifyOrQuit(msgClone != nullptr);
        recvContext->mProxyRxMsgs.Enqueue(*msgClone);
        break;
    default:
        Log("  Received unexpected URI %u", aUri);
        break;
    }

exit:
    return didHandle;
}

//---------------------------------------------------------------------------------------------------------------------

void TestBorderAdmitterEnrollerInteraction(void)
{
    static const char kEnrollerId[]    = "en00";
    static const char kEnrollerIdAlt[] = "en01";

    static const uint8_t kEnrollerTimeoutInSec = 50;

    Core                   nexus;
    Node                  &admitter = nexus.CreateNode();
    Node                  &enroller = nexus.CreateNode();
    Ip6::SockAddr          sockAddr;
    Pskc                   pskc;
    Admitter::Iterator     iter;
    Admitter::EnrollerInfo enrollerInfo;
    Admitter::JoinerInfo   joinerInfo;
    Coap::Message         *message;
    uint8_t                mode;
    MeshCoP::SteeringData  steeringData;
    MeshCoP::SteeringData  leaderSteeringData;
    ResponseContext        responseContext;
    ReceiveContext         recvContext;
    Manager::SessionInfo  *sessionInfo;
    uint16_t               rloc16;
    uint16_t               sessionId;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestBorderAdmitterEnrollerInteraction");

    nexus.AdvanceTime(0);

    // Form the topology:
    // - `admitter` forms its own network (acting as leader)
    // - `enroller` stays disconnected.

    admitter.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);
    VerifyOrQuit(admitter.Get<Mle::Mle>().IsLeader());

    SuccessOrQuit(enroller.Get<Mac::Mac>().SetPanChannel(admitter.Get<Mac::Mac>().GetPanChannel()));
    enroller.Get<Mac::Mac>().SetPanId(admitter.Get<Mac::Mac>().GetPanId());
    enroller.Get<ThreadNetif>().Up();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Enable Border Admitter on `admitter`");

    admitter.Get<Admitter>().SetEnabled(true);
    VerifyOrQuit(admitter.Get<Admitter>().IsEnabled());
    VerifyOrQuit(!admitter.Get<Admitter>().IsPrimeAdmitter());

    nexus.AdvanceTime(45 * Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(!admitter.Get<Admitter>().IsActiveCommissioner());

    SuccessOrQuit(admitter.Get<Ip6::Filter>().AddUnsecurePort(admitter.Get<Manager>().GetUdpPort()));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Establish a DTLS connection from `enroller` to `admitter`");

    sockAddr.SetAddress(admitter.Get<Mle::Mle>().GetLinkLocalAddress());
    sockAddr.SetPort(admitter.Get<Manager>().GetUdpPort());

    admitter.Get<KeyManager>().GetPskc(pskc);
    SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().SetPsk(pskc.m8, Pskc::kSize));

    enroller.Get<Tmf::SecureAgent>().RegisterResourceHandler(HandleResource, &recvContext);

    SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().Open());
    SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().Connect(sockAddr));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(enroller.Get<Tmf::SecureAgent>().IsConnected());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check that Enroller session list on `admitter` is empty");

    iter.Init(admitter.GetInstance());
    VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);
    VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send an `EnrollerRegister` message from `enroller` to `admitter`");

    message = enroller.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerRegister);
    VerifyOrQuit(message != nullptr);

    mode = MeshCoP::EnrollerModeTlv::kForwardJoinerRelayRx | MeshCoP::EnrollerModeTlv::kForwardUdpProxyRx;

    steeringData.SetToPermitAllJoiners();

    SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerIdTlv>(*message, kEnrollerId));
    SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerModeTlv>(*message, mode));
    SuccessOrQuit(Tlv::Append<MeshCoP::SteeringDataTlv>(*message, steeringData.GetData(), steeringData.GetLength()));

    responseContext.Clear();
    SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContext));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(responseContext.mReceived);
    VerifyOrQuit(responseContext.mResponseState == MeshCoP::StateTlv::kAccept);
    VerifyOrQuit(responseContext.mHasAdmitterState);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate the enroller list on `admitter`");

    iter.Init(admitter.GetInstance());
    SuccessOrQuit(iter.GetNextEnrollerInfo(enrollerInfo));

    sessionInfo = &enrollerInfo.mSessionInfo;
    VerifyOrQuit(sessionInfo->mIsConnected);
    VerifyOrQuit(!sessionInfo->mIsCommissioner);
    VerifyOrQuit(enroller.Get<ThreadNetif>().HasUnicastAddress(AsCoreType(&sessionInfo->mPeerSockAddr.mAddress)));

    VerifyOrQuit(StringMatch(enrollerInfo.mId, kEnrollerId));
    VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
    VerifyOrQuit(enrollerInfo.mMode == mode);

    VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);

    VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that `admitter` becomes active commissioner");

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(admitter.Get<Admitter>().IsActiveCommissioner());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that `EnrollerReportState` is received with the updated Admitter state");

    VerifyOrQuit(recvContext.HasReceivedReportState());
    VerifyOrQuit(recvContext.GetLastReportedAdmitterState() == kAdmitterActive);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that commissioner steering data and session ID are properly set");

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindBorderAgentRloc(rloc16));
    VerifyOrQuit(rloc16 == admitter.Get<Mle::Mle>().GetRloc16());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindCommissioningSessionId(sessionId));
    VerifyOrQuit(sessionId == admitter.Get<Admitter>().GetCommissionerSessionId());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindSteeringData(leaderSteeringData));
    VerifyOrQuit(leaderSteeringData == steeringData);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Ensure no changes before enroller timeout");

    recvContext.Clear();

    nexus.AdvanceTime((kEnrollerTimeoutInSec - 3) * Time::kOneSecondInMsec);

    VerifyOrQuit(!recvContext.HasReceivedReportState());

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(admitter.Get<Admitter>().IsActiveCommissioner());

    iter.Init(admitter.GetInstance());
    SuccessOrQuit(iter.GetNextEnrollerInfo(enrollerInfo));

    VerifyOrQuit(StringMatch(enrollerInfo.mId, kEnrollerId));
    VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
    VerifyOrQuit(enrollerInfo.mMode == mode);

    VerifyOrQuit(enrollerInfo.mRegisterDuration >= 48 * Time::kOneSecondInMsec);

    VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);

    VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send an `EnrollerKeepAlive` message");

    message = enroller.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerKeepAlive);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(Tlv::Append<MeshCoP::StateTlv>(*message, MeshCoP::StateTlv::kAccept));

    responseContext.Clear();
    SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContext));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(responseContext.mReceived);
    VerifyOrQuit(responseContext.mResponseState == MeshCoP::StateTlv::kAccept);
    VerifyOrQuit(responseContext.mHasAdmitterState);
    VerifyOrQuit(responseContext.mAdmitterState == kAdmitterActive);

    iter.Init(admitter.GetInstance());
    SuccessOrQuit(iter.GetNextEnrollerInfo(enrollerInfo));

    VerifyOrQuit(StringMatch(enrollerInfo.mId, kEnrollerId));
    VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
    VerifyOrQuit(enrollerInfo.mMode == mode);

    VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);

    VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);
    VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);

    VerifyOrQuit(!recvContext.HasReceivedReportState());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Wait until just before the timeout and validate that `admitter` remains active commissioner");

    nexus.AdvanceTime((kEnrollerTimeoutInSec - 2) * Time::kOneSecondInMsec);

    VerifyOrQuit(!recvContext.HasReceivedReportState());

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(admitter.Get<Admitter>().IsActiveCommissioner());

    iter.Init(admitter.GetInstance());
    SuccessOrQuit(iter.GetNextEnrollerInfo(enrollerInfo));
    VerifyOrQuit(StringMatch(enrollerInfo.mId, kEnrollerId));
    VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send an `EnrollerKeepAlive` message with an Enroller Mode TLV changing the mode");

    message = enroller.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerKeepAlive);
    VerifyOrQuit(message != nullptr);

    mode = MeshCoP::EnrollerModeTlv::kForwardJoinerRelayRx;

    SuccessOrQuit(Tlv::Append<MeshCoP::StateTlv>(*message, MeshCoP::StateTlv::kAccept));
    SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerModeTlv>(*message, mode));

    responseContext.Clear();
    SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContext));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(responseContext.mReceived);
    VerifyOrQuit(responseContext.mResponseState == MeshCoP::StateTlv::kAccept);
    VerifyOrQuit(responseContext.mHasAdmitterState);
    VerifyOrQuit(responseContext.mAdmitterState == kAdmitterActive);

    iter.Init(admitter.GetInstance());
    SuccessOrQuit(iter.GetNextEnrollerInfo(enrollerInfo));

    VerifyOrQuit(StringMatch(enrollerInfo.mId, kEnrollerId));
    VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
    VerifyOrQuit(enrollerInfo.mMode == mode);

    VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);

    VerifyOrQuit(!recvContext.HasReceivedReportState());

    nexus.AdvanceTime((kEnrollerTimeoutInSec - 2) * Time::kOneSecondInMsec);

    VerifyOrQuit(!recvContext.HasReceivedReportState());

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(admitter.Get<Admitter>().IsActiveCommissioner());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send an `EnrollerKeepAlive` message with Steering Data TLV");

    message = enroller.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerKeepAlive);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(steeringData.Init(MeshCoP::SteeringData::kMaxLength));
    SuccessOrQuit(steeringData.UpdateBloomFilter(admitter.Get<Mac::Mac>().GetExtAddress()));

    SuccessOrQuit(Tlv::Append<MeshCoP::StateTlv>(*message, MeshCoP::StateTlv::kAccept));
    SuccessOrQuit(Tlv::Append<MeshCoP::SteeringDataTlv>(*message, steeringData.GetData(), steeringData.GetLength()));

    responseContext.Clear();
    SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContext));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(responseContext.mReceived);
    VerifyOrQuit(responseContext.mResponseState == MeshCoP::StateTlv::kAccept);
    VerifyOrQuit(responseContext.mHasAdmitterState);
    VerifyOrQuit(responseContext.mAdmitterState == kAdmitterActive);

    iter.Init(admitter.GetInstance());
    SuccessOrQuit(iter.GetNextEnrollerInfo(enrollerInfo));

    VerifyOrQuit(StringMatch(enrollerInfo.mId, kEnrollerId));
    VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
    VerifyOrQuit(enrollerInfo.mMode == mode);

    VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate Network Data is updated with the new Steering Data");

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(!recvContext.HasReceivedReportState());

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(admitter.Get<Admitter>().IsActiveCommissioner());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindBorderAgentRloc(rloc16));
    VerifyOrQuit(rloc16 == admitter.Get<Mle::Mle>().GetRloc16());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindCommissioningSessionId(sessionId));
    VerifyOrQuit(sessionId == admitter.Get<Admitter>().GetCommissionerSessionId());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindSteeringData(leaderSteeringData));
    VerifyOrQuit(leaderSteeringData == steeringData);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate the enroller timeout");

    // Already 2 seconds has passed since sending last `EnrollerKeepAlive`
    nexus.AdvanceTime((kEnrollerTimeoutInSec - 2) * Time::kOneSecondInMsec - 50);

    VerifyOrQuit(!recvContext.HasReceivedReportState());

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(admitter.Get<Admitter>().IsActiveCommissioner());

    iter.Init(admitter.GetInstance());
    SuccessOrQuit(iter.GetNextEnrollerInfo(enrollerInfo));
    VerifyOrQuit(StringMatch(enrollerInfo.mId, kEnrollerId));
    VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);

    nexus.AdvanceTime(75);

    iter.Init(admitter.GetInstance());
    VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that the `admitter` resigns from being active commissioner");

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(!admitter.Get<Admitter>().IsActiveCommissioner());

    VerifyOrQuit(admitter.Get<NetworkData::Leader>().FindBorderAgentRloc(rloc16) == kErrorNotFound);

    nexus.AdvanceTime(10 * Time::kOneSecondInMsec);

    VerifyOrQuit(!enroller.Get<Tmf::SecureAgent>().IsConnected());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Re-establish DTLS session");

    SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().Connect(sockAddr));
    nexus.AdvanceTime(Time::kOneSecondInMsec);
    VerifyOrQuit(enroller.Get<Tmf::SecureAgent>().IsConnected());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Register as enroller again");

    message = enroller.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerRegister);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerIdTlv>(*message, kEnrollerId));
    SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerModeTlv>(*message, mode));
    SuccessOrQuit(Tlv::Append<MeshCoP::SteeringDataTlv>(*message, steeringData.GetData(), steeringData.GetLength()));

    responseContext.Clear();
    SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContext));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(responseContext.mReceived);
    VerifyOrQuit(responseContext.mResponseState == MeshCoP::StateTlv::kAccept);
    VerifyOrQuit(responseContext.mHasAdmitterState);

    iter.Init(admitter.GetInstance());
    SuccessOrQuit(iter.GetNextEnrollerInfo(enrollerInfo));
    VerifyOrQuit(StringMatch(enrollerInfo.mId, kEnrollerId));
    VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
    VerifyOrQuit(enrollerInfo.mMode == mode);
    VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that `admitter` becomes active commissioner again");

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(admitter.Get<Admitter>().IsActiveCommissioner());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindBorderAgentRloc(rloc16));
    VerifyOrQuit(rloc16 == admitter.Get<Mle::Mle>().GetRloc16());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindCommissioningSessionId(sessionId));
    VerifyOrQuit(sessionId == admitter.Get<Admitter>().GetCommissionerSessionId());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindSteeringData(leaderSteeringData));
    VerifyOrQuit(leaderSteeringData == steeringData);

    nexus.AdvanceTime((kEnrollerTimeoutInSec / 2) * Time::kOneSecondInMsec);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send an `EnrollerKeepAlive` message from `enroller` with `kReject` status, resigning enroller role");

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(admitter.Get<Admitter>().IsActiveCommissioner());

    message = enroller.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerKeepAlive);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(Tlv::Append<MeshCoP::StateTlv>(*message, MeshCoP::StateTlv::kReject));

    responseContext.Clear();
    SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContext));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(responseContext.mReceived);
    VerifyOrQuit(responseContext.mResponseState == MeshCoP::StateTlv::kReject);

    iter.Init(admitter.GetInstance());
    VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(!admitter.Get<Admitter>().IsActiveCommissioner());

    VerifyOrQuit(enroller.Get<Tmf::SecureAgent>().IsConnected());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Register as enroller again");

    message = enroller.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerRegister);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerIdTlv>(*message, kEnrollerId));
    SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerModeTlv>(*message, mode));
    SuccessOrQuit(Tlv::Append<MeshCoP::SteeringDataTlv>(*message, steeringData.GetData(), steeringData.GetLength()));

    responseContext.Clear();
    SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContext));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(responseContext.mReceived);
    VerifyOrQuit(responseContext.mResponseState == MeshCoP::StateTlv::kAccept);
    VerifyOrQuit(responseContext.mHasAdmitterState);

    iter.Init(admitter.GetInstance());
    SuccessOrQuit(iter.GetNextEnrollerInfo(enrollerInfo));
    VerifyOrQuit(StringMatch(enrollerInfo.mId, kEnrollerId));
    VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
    VerifyOrQuit(enrollerInfo.mMode == mode);
    VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);

    nexus.AdvanceTime((kEnrollerTimeoutInSec / 2) * Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(admitter.Get<Admitter>().IsActiveCommissioner());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send an `EnrollerRegister` message while already registered, with different parameters");

    message = enroller.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerRegister);
    VerifyOrQuit(message != nullptr);

    mode = 0;
    SuccessOrQuit(steeringData.Init(8));
    SuccessOrQuit(steeringData.UpdateBloomFilter(admitter.Get<Mac::Mac>().GetExtAddress()));

    SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerIdTlv>(*message, kEnrollerIdAlt));
    SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerModeTlv>(*message, mode));
    SuccessOrQuit(Tlv::Append<MeshCoP::SteeringDataTlv>(*message, steeringData.GetData(), steeringData.GetLength()));

    responseContext.Clear();
    SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContext));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(responseContext.mReceived);
    VerifyOrQuit(responseContext.mResponseState == MeshCoP::StateTlv::kAccept);

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(admitter.Get<Admitter>().IsActiveCommissioner());

    Log("Validate that enroller info is updated accordingly");

    iter.Init(admitter.GetInstance());
    SuccessOrQuit(iter.GetNextEnrollerInfo(enrollerInfo));

    VerifyOrQuit(StringMatch(enrollerInfo.mId, kEnrollerIdAlt));
    VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
    VerifyOrQuit(enrollerInfo.mMode == mode);

    VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that the Network Data (Commissioner Data) is also updated");

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindBorderAgentRloc(rloc16));
    VerifyOrQuit(rloc16 == admitter.Get<Mle::Mle>().GetRloc16());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindCommissioningSessionId(sessionId));
    VerifyOrQuit(sessionId == admitter.Get<Admitter>().GetCommissionerSessionId());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindSteeringData(leaderSteeringData));
    VerifyOrQuit(leaderSteeringData == steeringData);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that the `EnrollerRegister` extended the keep-alive timeout");

    recvContext.Clear();

    nexus.AdvanceTime((kEnrollerTimeoutInSec - 3) * Time::kOneSecondInMsec);

    VerifyOrQuit(!recvContext.HasReceivedReportState());

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(admitter.Get<Admitter>().IsActiveCommissioner());

    iter.Init(admitter.GetInstance());
    SuccessOrQuit(iter.GetNextEnrollerInfo(enrollerInfo));
    VerifyOrQuit(StringMatch(enrollerInfo.mId, kEnrollerIdAlt));
    VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send an invalid `EnrollerKeepAlive` message without State TLV and validate that it is rejected");

    message = enroller.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerKeepAlive);
    VerifyOrQuit(message != nullptr);

    responseContext.Clear();
    SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContext));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(responseContext.mReceived);
    VerifyOrQuit(responseContext.mResponseState == MeshCoP::StateTlv::kReject);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check that enroller is removed on `admitter`, and it stops being active commissioner");

    iter.Init(admitter.GetInstance());
    VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);

    nexus.AdvanceTime(10 * Time::kOneSecondInMsec);

    VerifyOrQuit(!enroller.Get<Tmf::SecureAgent>().IsConnected());

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(!admitter.Get<Admitter>().IsActiveCommissioner());

    VerifyOrQuit(admitter.Get<NetworkData::Leader>().FindBorderAgentRloc(rloc16) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Re-establish DTLS session");

    SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().Connect(sockAddr));
    nexus.AdvanceTime(Time::kOneSecondInMsec);
    VerifyOrQuit(enroller.Get<Tmf::SecureAgent>().IsConnected());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send `EnrollerRegister` with missing TLVs, validate that it is rejected");

    for (uint16_t testIter = 0; testIter < 3; testIter++)
    {
        message = enroller.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerRegister);
        VerifyOrQuit(message != nullptr);

        // Skip one of the required TLVs for each `testIter`.

        if (testIter != 0)
        {
            SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerIdTlv>(*message, kEnrollerId));
        }

        if (testIter != 1)
        {
            SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerModeTlv>(*message, mode));
        }

        if (testIter != 2)
        {
            SuccessOrQuit(
                Tlv::Append<MeshCoP::SteeringDataTlv>(*message, steeringData.GetData(), steeringData.GetLength()));
        }

        responseContext.Clear();
        SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContext));

        nexus.AdvanceTime(250);

        VerifyOrQuit(responseContext.mReceived);
        VerifyOrQuit(responseContext.mResponseState == MeshCoP::StateTlv::kReject);

        iter.Init(admitter.GetInstance());
        VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Log("Register as enroller with invalid Steering Data, validate that Admitter rejects");

    for (uint8_t length = 1; length <= 16; length++)
    {
        // Steering Data Length 1 can only be used with
        // `SetPermitAllJoiners()` or empty. Lengths of 8
        // and 16 are valid.

        if ((length == 8) || (length == 16))
        {
            continue;
        }

        Log("Send `EnrollerRegister` with invalid Steering Data length %u, validate that it is rejected", length);

        SuccessOrQuit(steeringData.Init(length));
        SuccessOrQuit(steeringData.UpdateBloomFilter(admitter.Get<Mac::Mac>().GetExtAddress()));

        message = enroller.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerRegister);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerIdTlv>(*message, kEnrollerId));
        SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerModeTlv>(*message, mode));
        SuccessOrQuit(
            Tlv::Append<MeshCoP::SteeringDataTlv>(*message, steeringData.GetData(), steeringData.GetLength()));

        responseContext.Clear();
        SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContext));

        nexus.AdvanceTime(250);

        VerifyOrQuit(responseContext.mReceived);
        VerifyOrQuit(responseContext.mResponseState == MeshCoP::StateTlv::kReject);

        iter.Init(admitter.GetInstance());
        VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    Log("Register as enroller with empty Steering Data, validate that Admitter accepts");

    SuccessOrQuit(steeringData.Init(1));

    message = enroller.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerRegister);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerIdTlv>(*message, kEnrollerId));
    SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerModeTlv>(*message, mode));
    SuccessOrQuit(Tlv::Append<MeshCoP::SteeringDataTlv>(*message, steeringData.GetData(), steeringData.GetLength()));

    responseContext.Clear();
    SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContext));

    nexus.AdvanceTime(250);

    VerifyOrQuit(responseContext.mReceived);
    VerifyOrQuit(responseContext.mResponseState == MeshCoP::StateTlv::kAccept);

    iter.Init(admitter.GetInstance());
    SuccessOrQuit(iter.GetNextEnrollerInfo(enrollerInfo));
    VerifyOrQuit(StringMatch(enrollerInfo.mId, kEnrollerId));
    VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
    VerifyOrQuit(enrollerInfo.mMode == mode);
    VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);
}

//---------------------------------------------------------------------------------------------------------------------

void TestBorderAdmitterCommissionerConflictAndPetitionerRetry(void)
{
    static const char kEnrollerId[] = "TestEnroller1234";

    static const uint8_t kEnrollerTimeoutInSec = 50;

    Core                   nexus;
    Node                  &admitter   = nexus.CreateNode();
    Node                  &enroller   = nexus.CreateNode();
    Node                  &otherCommr = nexus.CreateNode();
    Ip6::SockAddr          sockAddr;
    Pskc                   pskc;
    Admitter::Iterator     iter;
    Admitter::EnrollerInfo enrollerInfo;
    Coap::Message         *message;
    uint8_t                mode;
    MeshCoP::SteeringData  steeringData;
    MeshCoP::SteeringData  leaderSteeringData;
    ReceiveContext         recvContext;
    uint16_t               rloc16;
    uint16_t               sessionId;
    uint16_t               numStateChanges;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestBorderAdmitterCommissionerConflictAndPetitionerRetry");

    nexus.AdvanceTime(0);

    // Form the topology:
    // - `admitter` forms the network (as leader)
    // - `otherCommr` joins the same network.
    // - `enroller` stays disconnected.

    admitter.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);

    otherCommr.Join(admitter);

    nexus.AdvanceTime(10 * Time::kOneMinuteInMsec);

    VerifyOrQuit(admitter.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(otherCommr.Get<Mle::Mle>().IsRouter());

    SuccessOrQuit(enroller.Get<Mac::Mac>().SetPanChannel(admitter.Get<Mac::Mac>().GetPanChannel()));
    enroller.Get<Mac::Mac>().SetPanId(admitter.Get<Mac::Mac>().GetPanId());
    enroller.Get<ThreadNetif>().Up();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Enable Border Admitter on `admitter`");

    admitter.Get<Admitter>().SetEnabled(true);
    VerifyOrQuit(admitter.Get<Admitter>().IsEnabled());
    VerifyOrQuit(!admitter.Get<Admitter>().IsPrimeAdmitter());

    nexus.AdvanceTime(45 * Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(!admitter.Get<Admitter>().IsActiveCommissioner());

    SuccessOrQuit(admitter.Get<Ip6::Filter>().AddUnsecurePort(admitter.Get<Manager>().GetUdpPort()));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Make `otherCommr` the active commissioner");

    SuccessOrQuit(otherCommr.Get<Commissioner>().Start(nullptr, nullptr, nullptr));

    nexus.AdvanceTime(2 * Time::kOneSecondInMsec);

    VerifyOrQuit(otherCommr.Get<Commissioner>().GetState() == Commissioner::kStateActive);

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindBorderAgentRloc(rloc16));
    VerifyOrQuit(rloc16 == otherCommr.Get<Mle::Mle>().GetRloc16());

    nexus.AdvanceTime(5 * Time::kOneSecondInMsec);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Establish a DTLS connection from `enroller` to `admitter`");

    sockAddr.SetAddress(admitter.Get<Mle::Mle>().GetLinkLocalAddress());
    sockAddr.SetPort(admitter.Get<Manager>().GetUdpPort());

    admitter.Get<KeyManager>().GetPskc(pskc);
    SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().SetPsk(pskc.m8, Pskc::kSize));

    enroller.Get<Tmf::SecureAgent>().RegisterResourceHandler(HandleResource, &recvContext);

    SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().Open());
    SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().Connect(sockAddr));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(enroller.Get<Tmf::SecureAgent>().IsConnected());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send an `EnrollerRegister` message from `enroller` to `admitter`");

    message = enroller.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerRegister);
    VerifyOrQuit(message != nullptr);

    mode = MeshCoP::EnrollerModeTlv::kForwardJoinerRelayRx | MeshCoP::EnrollerModeTlv::kForwardUdpProxyRx;

    steeringData.SetToPermitAllJoiners();

    SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerIdTlv>(*message, kEnrollerId));
    SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerModeTlv>(*message, mode));
    SuccessOrQuit(Tlv::Append<MeshCoP::SteeringDataTlv>(*message, steeringData.GetData(), steeringData.GetLength()));

    SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().SendMessage(*message));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate the enroller list on `admitter`");

    iter.Init(admitter.GetInstance());
    SuccessOrQuit(iter.GetNextEnrollerInfo(enrollerInfo));

    VerifyOrQuit(StringMatch(enrollerInfo.mId, kEnrollerId));
    VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
    VerifyOrQuit(enrollerInfo.mMode == mode);

    VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Since there is another commissioner active, validate that `admitter` fails to become active");

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(!admitter.Get<Admitter>().IsActiveCommissioner());
    VerifyOrQuit(admitter.Get<Admitter>().IsPetitionRejected());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that `EnrollerReportState` is received with `ConflictError` state");

    VerifyOrQuit(recvContext.HasReceivedReportState());
    VerifyOrQuit(recvContext.GetLastReportedAdmitterState() == kAdmitterConflictError);

    nexus.AdvanceTime(10 * Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(!admitter.Get<Admitter>().IsActiveCommissioner());
    VerifyOrQuit(admitter.Get<Admitter>().IsPetitionRejected());

    VerifyOrQuit(otherCommr.Get<Commissioner>().GetState() == Commissioner::kStateActive);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Stop `otherCommr` from acting as active commissioner");

    recvContext.Clear();

    SuccessOrQuit(otherCommr.Get<Commissioner>().Stop());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that the `admitter` will detect this, petitions, and becomes active commissioner");

    nexus.AdvanceTime(2 * Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(admitter.Get<Admitter>().IsActiveCommissioner());
    VerifyOrQuit(!admitter.Get<Admitter>().IsPetitionRejected());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that `EnrollerReportState` is received now with `Active` state");

    VerifyOrQuit(recvContext.HasReceivedReportState());
    VerifyOrQuit(recvContext.GetLastReportedAdmitterState() == kAdmitterActive);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check the Network Data (Commissioner Data) to be properly set");

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindBorderAgentRloc(rloc16));
    VerifyOrQuit(rloc16 == admitter.Get<Mle::Mle>().GetRloc16());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindCommissioningSessionId(sessionId));
    VerifyOrQuit(sessionId == admitter.Get<Admitter>().GetCommissionerSessionId());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindSteeringData(leaderSteeringData));
    VerifyOrQuit(leaderSteeringData == steeringData);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("From `otherCommr` forcefully evict the current active commissioner (`admitter`)");

    recvContext.Clear();

    SuccessOrQuit(otherCommr.Get<Manager>().EvictActiveCommissioner());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send `EnrollerKeepAlive` three times, 20 seconds apart to maintain the enroller connection");

    for (uint8_t i = 0; i < 3; i++)
    {
        nexus.AdvanceTime(20 * Time::kOneSecondInMsec);

        message = enroller.Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerKeepAlive);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::StateTlv>(*message, MeshCoP::StateTlv::kAccept));

        SuccessOrQuit(enroller.Get<Tmf::SecureAgent>().SendMessage(*message));

        nexus.AdvanceTime(Time::kOneSecondInMsec);

        iter.Init(admitter.GetInstance());
        SuccessOrQuit(iter.GetNextEnrollerInfo(enrollerInfo));

        VerifyOrQuit(StringMatch(enrollerInfo.mId, kEnrollerId));
        VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
        VerifyOrQuit(enrollerInfo.mMode == mode);

        VerifyOrQuit(iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNotFound);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that the eviction was properly detected, and petitioner retry mechanism did restore it");

    VerifyOrQuit(recvContext.HasReceivedReportState());
    VerifyOrQuit(recvContext.GetLastReportedAdmitterState() == kAdmitterActive);

    numStateChanges = recvContext.mStateReports.GetLength();
    VerifyOrQuit(numStateChanges >= 2);
    VerifyOrQuit(recvContext.mStateReports[numStateChanges - 2].mAdmitterState == kAdmitterReady);
    VerifyOrQuit(recvContext.mStateReports[numStateChanges - 1].mAdmitterState == kAdmitterActive);
}

//---------------------------------------------------------------------------------------------------------------------

template <uint8_t kNumEnrollers>
uint8_t FindMatchingEnroller(const Admitter::EnrollerInfo &aInfo,
                             const char *(&aEnrollerIds)[kNumEnrollers],
                             BitSet<kNumEnrollers> &aFoundIndexes)
{
    // Finds a matching enroller by comparing `aInfo.mId` against
    // `aEnrollerIds[]` and returns the matched index. This function
    // ensures each enroller is found only once by checking that the
    // index is not already in `aFoundIndexes`, then updates
    // `aFoundIndexes`.

    uint8_t matchedIndex = kNumEnrollers;

    for (uint8_t index = 0; index < kNumEnrollers; index++)
    {
        if (StringMatch(aInfo.mId, aEnrollerIds[index]))
        {
            matchedIndex = index;
            break;
        }
    }

    VerifyOrQuit(matchedIndex < kNumEnrollers);

    VerifyOrQuit(!aFoundIndexes.Has(matchedIndex));
    aFoundIndexes.Add(matchedIndex);

    return matchedIndex;
}

template <uint8_t kNumEnrollers> bool DidFindAllEnrollers(const BitSet<kNumEnrollers> &aFoundIndexes)
{
    bool didFindAll = true;

    for (uint8_t index = 0; index < kNumEnrollers; index++)
    {
        if (!aFoundIndexes.Has(index))
        {
            didFindAll = false;
            break;
        }
    }

    return didFindAll;
}

//---------------------------------------------------------------------------------------------------------------------

void LogEnroller(const Admitter::EnrollerInfo &aInfo)
{
    Log("   Enroller - id:%s steeringData:%s mode:0x%02x", aInfo.mId,
        AsCoreType(&aInfo.mSteeringData).ToString().AsCString(), aInfo.mMode);
}

void LogJoiner(const Admitter::JoinerInfo &aInfo)
{
    Log("      Joiner - iid:%s, msec-till-expire:%lu", AsCoreType(&aInfo.mIid).ToString().AsCString(),
        ToUlong(aInfo.mMsecTillExpiration));
}

//---------------------------------------------------------------------------------------------------------------------

void TestBorderAdmitterMultipleEnrollers(void)
{
    static constexpr uint8_t kNumEnrollers = 4;

    static const char *kEnrollerIds[kNumEnrollers] = {"earth", "water", "wind", "fire"};

    Core                   nexus;
    Node                  &admitter = nexus.CreateNode();
    Node                  *enrollers[kNumEnrollers];
    Ip6::SockAddr          sockAddr;
    Pskc                   pskc;
    Admitter::Iterator     iter;
    Admitter::EnrollerInfo enrollerInfo;
    Admitter::JoinerInfo   joinerInfo;
    Coap::Message         *message;
    uint8_t                mode;
    MeshCoP::SteeringData  steeringData[kNumEnrollers];
    ReceiveContext         recvContext[kNumEnrollers];
    ResponseContext        responseContexts[kNumEnrollers];
    BitSet<kNumEnrollers>  foundEnrollers;
    MeshCoP::SteeringData  leaderSteeringData;
    MeshCoP::SteeringData  combinedSteeringData;
    uint16_t               rloc16;
    uint16_t               sessionId;
    Mac::ExtAddress        joinerIid;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestBorderAdmitterMultipleEnrollers");

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        enrollers[i] = &nexus.CreateNode();
    }

    nexus.AdvanceTime(0);

    // Form the topology:
    // - `admitter` forms the network (as leader)
    // - All enrollers stay disconnected.

    admitter.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Mle::Mle>().IsLeader());

    for (Node *enroller : enrollers)
    {
        SuccessOrQuit(enroller->Get<Mac::Mac>().SetPanChannel(admitter.Get<Mac::Mac>().GetPanChannel()));
        enroller->Get<Mac::Mac>().SetPanId(admitter.Get<Mac::Mac>().GetPanId());
        enroller->Get<ThreadNetif>().Up();
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Enable Border Admitter on `admitter`");

    admitter.Get<Admitter>().SetEnabled(true);
    VerifyOrQuit(admitter.Get<Admitter>().IsEnabled());
    VerifyOrQuit(!admitter.Get<Admitter>().IsPrimeAdmitter());

    nexus.AdvanceTime(45 * Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(!admitter.Get<Admitter>().IsActiveCommissioner());

    SuccessOrQuit(admitter.Get<Ip6::Filter>().AddUnsecurePort(admitter.Get<Manager>().GetUdpPort()));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Establish a DTLS connection from all `enrollers` to `admitter`");

    sockAddr.SetAddress(admitter.Get<Mle::Mle>().GetLinkLocalAddress());
    sockAddr.SetPort(admitter.Get<Manager>().GetUdpPort());

    admitter.Get<KeyManager>().GetPskc(pskc);

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        Node *enroller = enrollers[i];

        SuccessOrQuit(enroller->Get<Tmf::SecureAgent>().SetPsk(pskc.m8, Pskc::kSize));

        recvContext[i].Clear();
        enroller->Get<Tmf::SecureAgent>().RegisterResourceHandler(HandleResource, &recvContext[i]);

        SuccessOrQuit(enroller->Get<Tmf::SecureAgent>().Open());
        SuccessOrQuit(enroller->Get<Tmf::SecureAgent>().Connect(sockAddr));

        nexus.AdvanceTime(Time::kOneSecondInMsec);

        VerifyOrQuit(enroller->Get<Tmf::SecureAgent>().IsConnected());
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Prepare Steering Data for each enroller");

    mode = MeshCoP::EnrollerModeTlv::kForwardJoinerRelayRx | MeshCoP::EnrollerModeTlv::kForwardUdpProxyRx;

    steeringData[0].SetToPermitAllJoiners();

    SuccessOrQuit(steeringData[1].Init(16));

    for (uint8_t numJoiners = 0; numJoiners < 3; numJoiners++)
    {
        joinerIid.GenerateRandom();
        SuccessOrQuit(steeringData[1].UpdateBloomFilter(joinerIid));
    }

    SuccessOrQuit(steeringData[2].Init(8));
    joinerIid.GenerateRandom();
    SuccessOrQuit(steeringData[2].UpdateBloomFilter(joinerIid));

    SuccessOrQuit(steeringData[3].Init(8));
    joinerIid.GenerateRandom();
    SuccessOrQuit(steeringData[3].UpdateBloomFilter(joinerIid));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send an `EnrollerRegister` message from all `enrollers`");

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        message = enrollers[i]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerRegister);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerIdTlv>(*message, kEnrollerIds[i]));
        SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerModeTlv>(*message, mode));
        SuccessOrQuit(
            Tlv::Append<MeshCoP::SteeringDataTlv>(*message, steeringData[i].GetData(), steeringData[i].GetLength()));

        responseContexts[i].Clear();
        SuccessOrQuit(
            enrollers[i]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[i]));
    }

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check that all registrations were accepted");

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        VerifyOrQuit(responseContexts[i].mReceived);
        VerifyOrQuit(responseContexts[i].mResponseState == MeshCoP::StateTlv::kAccept);
        VerifyOrQuit(responseContexts[i].mHasAdmitterState);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that `admitter` becomes active commissioner");

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(admitter.Get<Admitter>().IsActiveCommissioner());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindBorderAgentRloc(rloc16));
    VerifyOrQuit(rloc16 == admitter.Get<Mle::Mle>().GetRloc16());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindCommissioningSessionId(sessionId));
    VerifyOrQuit(sessionId == admitter.Get<Admitter>().GetCommissionerSessionId());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindSteeringData(leaderSteeringData));
    VerifyOrQuit(leaderSteeringData.GetLength() == 1);
    VerifyOrQuit(leaderSteeringData.PermitsAllJoiners());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that `EnrollerReportState` is received with the updated Admitter state");

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        // Some enrollers may already get the updated state in the Register response
        if (recvContext[i].HasReceivedReportState())
        {
            VerifyOrQuit(recvContext[i].GetLastReportedAdmitterState() == kAdmitterActive);
        }
    }

    // - - - - - - - - - - - - - - - - - - - - - - -. - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate the enroller list on `admitter`");

    foundEnrollers.Clear();
    iter.Init(admitter.GetInstance());

    while (iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNone)
    {
        uint8_t matchedIndex;

        LogEnroller(enrollerInfo);

        matchedIndex = FindMatchingEnroller<kNumEnrollers>(enrollerInfo, kEnrollerIds, foundEnrollers);

        VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData[matchedIndex]);
        VerifyOrQuit(enrollerInfo.mMode == mode);

        VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);
    }

    VerifyOrQuit(DidFindAllEnrollers<kNumEnrollers>(foundEnrollers));

    nexus.AdvanceTime(10 * Time::kOneSecondInMsec);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send a keep alive from first enroller with reject status (to unregister the enroller)");

    message = enrollers[0]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerKeepAlive);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(Tlv::Append<MeshCoP::StateTlv>(*message, MeshCoP::StateTlv::kReject));

    responseContexts[0].Clear();
    SuccessOrQuit(enrollers[0]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[0]));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(responseContexts[0].mReceived);
    VerifyOrQuit(responseContexts[0].mResponseState == MeshCoP::StateTlv::kReject);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check enroller info and that enroller 0 is no longer present");

    foundEnrollers.Clear();

    iter.Init(admitter.GetInstance());

    while (iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNone)
    {
        uint8_t matchedIndex;

        LogEnroller(enrollerInfo);

        matchedIndex = FindMatchingEnroller<kNumEnrollers>(enrollerInfo, kEnrollerIds, foundEnrollers);
        VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData[matchedIndex]);
        VerifyOrQuit(enrollerInfo.mMode == mode);

        VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);
    }

    VerifyOrQuit(!foundEnrollers.Has(0));
    VerifyOrQuit(foundEnrollers.Has(1));
    VerifyOrQuit(foundEnrollers.Has(2));
    VerifyOrQuit(foundEnrollers.Has(3));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that `admitter` is still active commissioner and the steering data is updated");

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(admitter.Get<Admitter>().IsActiveCommissioner());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindBorderAgentRloc(rloc16));
    VerifyOrQuit(rloc16 == admitter.Get<Mle::Mle>().GetRloc16());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindCommissioningSessionId(sessionId));
    VerifyOrQuit(sessionId == admitter.Get<Admitter>().GetCommissionerSessionId());

    combinedSteeringData = steeringData[1];
    SuccessOrQuit(combinedSteeringData.MergeBloomFilterWith(steeringData[2]));
    SuccessOrQuit(combinedSteeringData.MergeBloomFilterWith(steeringData[3]));

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindSteeringData(leaderSteeringData));
    VerifyOrQuit(leaderSteeringData == combinedSteeringData);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Wait 20 seconds, send keep-alive from enrollers[2,3] but not from [1]");

    nexus.AdvanceTime(20 * Time::kOneSecondInMsec);

    for (uint8_t i = 2; i < kNumEnrollers; i++)
    {
        message = enrollers[i]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerKeepAlive);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::StateTlv>(*message, MeshCoP::StateTlv::kAccept));

        responseContexts[i].Clear();
        SuccessOrQuit(
            enrollers[i]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[i]));
    }

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    for (uint8_t i = 2; i < kNumEnrollers; i++)
    {
        VerifyOrQuit(responseContexts[i].mReceived);
        VerifyOrQuit(responseContexts[i].mResponseState == MeshCoP::StateTlv::kAccept);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Wait 35 more seconds and check that enroller 1 is now removed");

    nexus.AdvanceTime(35 * Time::kOneSecondInMsec);

    foundEnrollers.Clear();

    iter.Init(admitter.GetInstance());

    while (iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNone)
    {
        LogEnroller(enrollerInfo);

        FindMatchingEnroller<kNumEnrollers>(enrollerInfo, kEnrollerIds, foundEnrollers);

        VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);
    }

    VerifyOrQuit(!foundEnrollers.Has(0));
    VerifyOrQuit(!foundEnrollers.Has(1));
    VerifyOrQuit(foundEnrollers.Has(2));
    VerifyOrQuit(foundEnrollers.Has(3));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that `admitter` updates the steering data");

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(admitter.Get<Admitter>().IsActiveCommissioner());

    combinedSteeringData = steeringData[2];
    SuccessOrQuit(combinedSteeringData.MergeBloomFilterWith(steeringData[3]));

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindSteeringData(leaderSteeringData));
    VerifyOrQuit(leaderSteeringData == combinedSteeringData);
}

//---------------------------------------------------------------------------------------------------------------------

void TestBorderAdmitterJoinerEnrollerInteraction(void)
{
    static constexpr uint8_t kNumEnrollers = 4;
    static constexpr uint8_t kNumJoiners   = 2;

    static const char *kEnrollerIds[kNumEnrollers] = {"diamond", "ruby", "sapphire", "emerald"};

    static const char kPskd[] = "J01NME1234";

    Core                     nexus;
    Node                    &admitter = nexus.CreateNode();
    Node                    *enrollers[kNumEnrollers];
    Node                    *joiners[kNumJoiners];
    Ip6::SockAddr            sockAddr;
    Pskc                     pskc;
    Coap::Message           *message;
    uint8_t                  modes[kNumEnrollers];
    ResponseContext          responseContexts[kNumEnrollers];
    ReceiveContext           recvContext[kNumEnrollers];
    MeshCoP::SteeringData    steeringData;
    MeshCoP::SteeringData    leaderSteeringData;
    Ip6::InterfaceIdentifier joinerIids[kNumJoiners];
    Ip6::InterfaceIdentifier wildcardJoinerIid;
    Admitter::Iterator       iter;
    Admitter::EnrollerInfo   enrollerInfo;
    Admitter::JoinerInfo     joinerInfo;
    BitSet<kNumEnrollers>    foundEnrollers;
    BitSet<kNumJoiners>      foundJoiners;
    uint16_t                 sessionId;
    uint16_t                 rloc16;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestBorderAdmitterJoinerEnrollerInteraction");

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        enrollers[i] = &nexus.CreateNode();
    }

    for (uint8_t j = 0; j < kNumJoiners; j++)
    {
        joiners[j] = &nexus.CreateNode();
    }

    nexus.AdvanceTime(0);

    // Form the topology:
    // - `admitter` forms the network (as leader)
    // - All enrollers stay disconnected.

    admitter.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Mle::Mle>().IsLeader());

    for (Node *enroller : enrollers)
    {
        SuccessOrQuit(enroller->Get<Mac::Mac>().SetPanChannel(admitter.Get<Mac::Mac>().GetPanChannel()));
        enroller->Get<Mac::Mac>().SetPanId(admitter.Get<Mac::Mac>().GetPanId());
        enroller->Get<ThreadNetif>().Up();
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Enable Border Admitter on `admitter`");

    admitter.Get<Admitter>().SetEnabled(true);
    VerifyOrQuit(admitter.Get<Admitter>().IsEnabled());
    VerifyOrQuit(!admitter.Get<Admitter>().IsPrimeAdmitter());

    nexus.AdvanceTime(45 * Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(!admitter.Get<Admitter>().IsActiveCommissioner());

    SuccessOrQuit(admitter.Get<Ip6::Filter>().AddUnsecurePort(admitter.Get<Manager>().GetUdpPort()));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Establish a DTLS connection from all `enrollers` to `admitter`");

    sockAddr.SetAddress(admitter.Get<Mle::Mle>().GetLinkLocalAddress());
    sockAddr.SetPort(admitter.Get<Manager>().GetUdpPort());

    admitter.Get<KeyManager>().GetPskc(pskc);

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        Node *enroller = enrollers[i];

        SuccessOrQuit(enroller->Get<Tmf::SecureAgent>().SetPsk(pskc.m8, Pskc::kSize));

        recvContext[i].Clear();
        enroller->Get<Tmf::SecureAgent>().RegisterResourceHandler(HandleResource, &recvContext[i]);

        SuccessOrQuit(enroller->Get<Tmf::SecureAgent>().Open());
        SuccessOrQuit(enroller->Get<Tmf::SecureAgent>().Connect(sockAddr));

        nexus.AdvanceTime(Time::kOneSecondInMsec);

        VerifyOrQuit(enroller->Get<Tmf::SecureAgent>().IsConnected());
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Prepare mode for each enroller");

    modes[0] = MeshCoP::EnrollerModeTlv::kForwardJoinerRelayRx | MeshCoP::EnrollerModeTlv::kForwardUdpProxyRx;
    modes[1] = MeshCoP::EnrollerModeTlv::kForwardJoinerRelayRx | MeshCoP::EnrollerModeTlv::kForwardUdpProxyRx;
    modes[2] = MeshCoP::EnrollerModeTlv::kForwardJoinerRelayRx;
    modes[3] = MeshCoP::EnrollerModeTlv::kForwardUdpProxyRx;

    steeringData.SetToPermitAllJoiners();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send an `EnrollerRegister` message from all `enrollers`");

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        message = enrollers[i]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerRegister);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerIdTlv>(*message, kEnrollerIds[i]));
        SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerModeTlv>(*message, modes[i]));
        SuccessOrQuit(
            Tlv::Append<MeshCoP::SteeringDataTlv>(*message, steeringData.GetData(), steeringData.GetLength()));

        responseContexts[i].Clear();
        SuccessOrQuit(
            enrollers[i]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[i]));
    }

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check that all registrations were accepted");

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        VerifyOrQuit(responseContexts[i].mReceived);
        VerifyOrQuit(responseContexts[i].mResponseState == MeshCoP::StateTlv::kAccept);
        VerifyOrQuit(responseContexts[i].mHasAdmitterState);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that `admitter` becomes active commissioner");

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(admitter.Get<Admitter>().IsActiveCommissioner());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindBorderAgentRloc(rloc16));
    VerifyOrQuit(rloc16 == admitter.Get<Mle::Mle>().GetRloc16());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindCommissioningSessionId(sessionId));
    VerifyOrQuit(sessionId == admitter.Get<Admitter>().GetCommissionerSessionId());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindSteeringData(leaderSteeringData));
    VerifyOrQuit(leaderSteeringData == steeringData);
    VerifyOrQuit(leaderSteeringData.PermitsAllJoiners());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate the Enroller info on `admitter`");

    foundEnrollers.Clear();
    iter.Init(admitter.GetInstance());

    while (iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNone)
    {
        uint8_t matchedIndex;

        LogEnroller(enrollerInfo);

        matchedIndex = FindMatchingEnroller<kNumEnrollers>(enrollerInfo, kEnrollerIds, foundEnrollers);

        VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
        VerifyOrQuit(enrollerInfo.mMode == modes[matchedIndex]);

        VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);
    }

    VerifyOrQuit(DidFindAllEnrollers<kNumEnrollers>(foundEnrollers));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Start `joiners[0]`");

    joiners[0]->Get<ThreadNetif>().Up();
    SuccessOrQuit(joiners[0]->Get<Joiner>().Start(kPskd,
                                                  /* aProvisioningUrl */ nullptr,
                                                  /* aVendorName */ nullptr,
                                                  /* aVendorModel */ nullptr,
                                                  /* aVendorSwVersion */ nullptr,
                                                  /* aVendorData */ nullptr,
                                                  /* aCallback */ nullptr,
                                                  /* aContext */ nullptr));

    joinerIids[0].SetFromExtAddress(joiners[0]->Get<Joiner>().GetId());

    nexus.AdvanceTime(8 * Time::kOneSecondInMsec);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that `joiner` `RelayRx` are forwarded to all `enrollers` with `kForwardJoinerRelayRx` mode flag");

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        Coap::Message           *message = AsCoapMessagePtr(recvContext[i].mRelayRxMsgs.GetHead());
        Ip6::InterfaceIdentifier readIid;
        uint16_t                 joinerRouterRloc;

        if ((modes[i] & MeshCoP::EnrollerModeTlv::kForwardJoinerRelayRx) == 0)
        {
            VerifyOrQuit(message == nullptr);
            continue;
        }

        VerifyOrQuit(message != nullptr);

        VerifyOrQuit(message->ReadType() == Coap::kTypeNonConfirmable);
        VerifyOrQuit(message->ReadCode() == Coap::kCodePost);
        SuccessOrQuit(Tlv::Find<MeshCoP::JoinerIidTlv>(*message, readIid));
        SuccessOrQuit(Tlv::Find<MeshCoP::JoinerRouterLocatorTlv>(*message, joinerRouterRloc));

        VerifyOrQuit(readIid == joinerIids[0]);
        VerifyOrQuit(joinerRouterRloc = admitter.Get<Mle::Mle>().GetRloc16());
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Stop `joiners[0]`");

    joiners[0]->Get<Joiner>().Stop();

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        recvContext[i].Clear();
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send an `EnrollerJoinerAccept` message from `enrollers[0]` to `admitter` accepting `joiners[0]`");

    message = enrollers[0]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerJoinerAccept);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(Tlv::Append<MeshCoP::JoinerIidTlv>(*message, joinerIids[0]));

    responseContexts[0].Clear();
    SuccessOrQuit(enrollers[0]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[0]));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(responseContexts[0].mReceived);
    VerifyOrQuit(responseContexts[0].mResponseState == MeshCoP::StateTlv::kAccept);
    VerifyOrQuit(!responseContexts[0].mHasAdmitterState);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that the accepted `joiners[0]` is tracked by `enrollers[0]` entry on `admitter`");

    foundEnrollers.Clear();
    iter.Init(admitter.GetInstance());

    while (iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNone)
    {
        uint8_t matchedIndex;

        LogEnroller(enrollerInfo);

        matchedIndex = FindMatchingEnroller<kNumEnrollers>(enrollerInfo, kEnrollerIds, foundEnrollers);

        VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
        VerifyOrQuit(enrollerInfo.mMode == modes[matchedIndex]);

        if (matchedIndex == 0)
        {
            SuccessOrQuit(iter.GetNextJoinerInfo(joinerInfo));
            VerifyOrQuit(AsCoreType(&joinerInfo.mIid) == joinerIids[0]);
            LogJoiner(joinerInfo);
        }

        VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);
    }

    VerifyOrQuit(DidFindAllEnrollers<kNumEnrollers>(foundEnrollers));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Start `joiners[0]` again and validate that its `RelayRx` are only forwarded to `enrollers[0]`");

    joiners[0]->Get<ThreadNetif>().Up();
    SuccessOrQuit(joiners[0]->Get<Joiner>().Start(kPskd,
                                                  /* aProvisioningUrl */ nullptr,
                                                  /* aVendorName */ nullptr,
                                                  /* aVendorModel */ nullptr,
                                                  /* aVendorSwVersion */ nullptr,
                                                  /* aVendorData */ nullptr,
                                                  /* aCallback */ nullptr,
                                                  /* aContext */ nullptr));

    nexus.AdvanceTime(8 * Time::kOneSecondInMsec);

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        Coap::Message           *message = AsCoapMessagePtr(recvContext[i].mRelayRxMsgs.GetHead());
        Ip6::InterfaceIdentifier readIid;
        uint16_t                 joinerRouterRloc;

        if (i != 0)
        {
            VerifyOrQuit(message == nullptr);
            continue;
        }

        VerifyOrQuit(message != nullptr);

        VerifyOrQuit(message->ReadType() == Coap::kTypeNonConfirmable);
        VerifyOrQuit(message->ReadCode() == Coap::kCodePost);
        SuccessOrQuit(Tlv::Find<MeshCoP::JoinerIidTlv>(*message, readIid));
        SuccessOrQuit(Tlv::Find<MeshCoP::JoinerRouterLocatorTlv>(*message, joinerRouterRloc));

        VerifyOrQuit(readIid == joinerIids[0]);
        VerifyOrQuit(joinerRouterRloc = admitter.Get<Mle::Mle>().GetRloc16());
    }

    joiners[0]->Get<Joiner>().Stop();

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        recvContext[i].Clear();
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Start `joiners[1]` and validate that its `RelayRx` are forwarded to all `enrollers`");

    joiners[1]->Get<ThreadNetif>().Up();
    SuccessOrQuit(joiners[1]->Get<Joiner>().Start(kPskd,
                                                  /* aProvisioningUrl */ nullptr,
                                                  /* aVendorName */ nullptr,
                                                  /* aVendorModel */ nullptr,
                                                  /* aVendorSwVersion */ nullptr,
                                                  /* aVendorData */ nullptr,
                                                  /* aCallback */ nullptr,
                                                  /* aContext */ nullptr));

    joinerIids[1].SetFromExtAddress(joiners[1]->Get<Joiner>().GetId());

    nexus.AdvanceTime(8 * Time::kOneSecondInMsec);

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        Coap::Message           *message = AsCoapMessagePtr(recvContext[i].mRelayRxMsgs.GetHead());
        Ip6::InterfaceIdentifier readIid;
        uint16_t                 joinerRouterRloc;

        if ((modes[i] & MeshCoP::EnrollerModeTlv::kForwardJoinerRelayRx) == 0)
        {
            VerifyOrQuit(message == nullptr);
            continue;
        }

        VerifyOrQuit(message != nullptr);

        VerifyOrQuit(message->ReadType() == Coap::kTypeNonConfirmable);
        VerifyOrQuit(message->ReadCode() == Coap::kCodePost);
        SuccessOrQuit(Tlv::Find<MeshCoP::JoinerIidTlv>(*message, readIid));
        SuccessOrQuit(Tlv::Find<MeshCoP::JoinerRouterLocatorTlv>(*message, joinerRouterRloc));

        VerifyOrQuit(readIid == joinerIids[1]);
        VerifyOrQuit(joinerRouterRloc = admitter.Get<Mle::Mle>().GetRloc16());
    }

    joiners[1]->Get<Joiner>().Stop();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send `EnrollerKeepAlive` message from all `enrollers` to maintain the connection");

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        message = enrollers[i]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerKeepAlive);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::StateTlv>(*message, MeshCoP::StateTlv::kAccept));

        responseContexts[i].Clear();
        SuccessOrQuit(
            enrollers[i]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[i]));
    }

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        VerifyOrQuit(responseContexts[i].mReceived);
        VerifyOrQuit(responseContexts[i].mResponseState == MeshCoP::StateTlv::kAccept);
        VerifyOrQuit(responseContexts[i].mHasAdmitterState);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send an `EnrollerJoinerAccept` message from `enrollers[0]` to `admitter` accepting `joiners[1]`");

    message = enrollers[0]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerJoinerAccept);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(Tlv::Append<MeshCoP::JoinerIidTlv>(*message, joinerIids[1]));

    responseContexts[0].Clear();
    SuccessOrQuit(enrollers[0]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[0]));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(responseContexts[0].mReceived);
    VerifyOrQuit(responseContexts[0].mResponseState == MeshCoP::StateTlv::kAccept);
    VerifyOrQuit(!responseContexts[0].mHasAdmitterState);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that both accepted `joiners` are tracked by `enrollers[0]` on `admitter`");

    foundEnrollers.Clear();
    iter.Init(admitter.GetInstance());

    while (iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNone)
    {
        uint8_t matchedIndex;

        LogEnroller(enrollerInfo);

        matchedIndex = FindMatchingEnroller<kNumEnrollers>(enrollerInfo, kEnrollerIds, foundEnrollers);

        VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
        VerifyOrQuit(enrollerInfo.mMode == modes[matchedIndex]);

        if (matchedIndex == 0)
        {
            uint16_t numJoiners = 0;

            foundJoiners.Clear();

            while (iter.GetNextJoinerInfo(joinerInfo) == kErrorNone)
            {
                LogJoiner(joinerInfo);

                numJoiners++;

                for (uint8_t j = 0; j < 2; j++)
                {
                    if (joinerIids[j] == AsCoreType(&joinerInfo.mIid))
                    {
                        VerifyOrQuit(!foundJoiners.Has(j));
                        foundJoiners.Add(j);
                    }
                }
            }

            VerifyOrQuit(numJoiners == 2);
        }
        else
        {
            VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);
        }
    }

    VerifyOrQuit(DidFindAllEnrollers<kNumEnrollers>(foundEnrollers));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Start `joiners[1]` again and validate that its `RelayRx` are only forwarded to `enrollers[0]`");

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        recvContext[i].Clear();
    }

    joiners[1]->Get<ThreadNetif>().Up();
    SuccessOrQuit(joiners[1]->Get<Joiner>().Start(kPskd,
                                                  /* aProvisioningUrl */ nullptr,
                                                  /* aVendorName */ nullptr,
                                                  /* aVendorModel */ nullptr,
                                                  /* aVendorSwVersion */ nullptr,
                                                  /* aVendorData */ nullptr,
                                                  /* aCallback */ nullptr,
                                                  /* aContext */ nullptr));

    nexus.AdvanceTime(8 * Time::kOneSecondInMsec);

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        Coap::Message           *message = AsCoapMessagePtr(recvContext[i].mRelayRxMsgs.GetHead());
        Ip6::InterfaceIdentifier readIid;
        uint16_t                 joinerRouterRloc;

        if (i != 0)
        {
            VerifyOrQuit(message == nullptr);
            continue;
        }

        VerifyOrQuit(message != nullptr);

        VerifyOrQuit(message->ReadType() == Coap::kTypeNonConfirmable);
        VerifyOrQuit(message->ReadCode() == Coap::kCodePost);
        SuccessOrQuit(Tlv::Find<MeshCoP::JoinerIidTlv>(*message, readIid));
        SuccessOrQuit(Tlv::Find<MeshCoP::JoinerRouterLocatorTlv>(*message, joinerRouterRloc));

        VerifyOrQuit(readIid == joinerIids[1]);
        VerifyOrQuit(joinerRouterRloc = admitter.Get<Mle::Mle>().GetRloc16());
    }

    joiners[1]->Get<Joiner>().Stop();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("From `enrollers[1]` send `EnrollerJoinerAccept` for `joiners[1]`");

    message = enrollers[1]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerJoinerAccept);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(Tlv::Append<MeshCoP::JoinerIidTlv>(*message, joinerIids[1]));

    responseContexts[1].Clear();
    SuccessOrQuit(enrollers[1]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[1]));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    Log("Validate that the request is rejected since `joiners[1]` is already accepted by `enrollers[0]`");

    VerifyOrQuit(responseContexts[1].mReceived);
    VerifyOrQuit(responseContexts[1].mResponseState == MeshCoP::StateTlv::kReject);
    VerifyOrQuit(!responseContexts[1].mHasAdmitterState);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate `joiners[1]` is still accepted by `enrollers[0]`");

    foundEnrollers.Clear();
    iter.Init(admitter.GetInstance());

    while (iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNone)
    {
        uint8_t matchedIndex;

        LogEnroller(enrollerInfo);

        matchedIndex = FindMatchingEnroller<kNumEnrollers>(enrollerInfo, kEnrollerIds, foundEnrollers);

        VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
        VerifyOrQuit(enrollerInfo.mMode == modes[matchedIndex]);

        if (matchedIndex == 0)
        {
            uint16_t numJoiners = 0;

            foundJoiners.Clear();

            while (iter.GetNextJoinerInfo(joinerInfo) == kErrorNone)
            {
                LogJoiner(joinerInfo);

                numJoiners++;

                for (uint8_t j = 0; j < 2; j++)
                {
                    if (joinerIids[j] == AsCoreType(&joinerInfo.mIid))
                    {
                        VerifyOrQuit(!foundJoiners.Has(j));
                        foundJoiners.Add(j);
                    }
                }
            }

            VerifyOrQuit(numJoiners == 2);
        }
        else
        {
            VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);
        }

        VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Start `joiners[1]` again and validate that its `RelayRx` are only forwarded to `enrollers[0]`");

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        recvContext[i].Clear();
    }

    joiners[1]->Get<ThreadNetif>().Up();
    SuccessOrQuit(joiners[1]->Get<Joiner>().Start(kPskd,
                                                  /* aProvisioningUrl */ nullptr,
                                                  /* aVendorName */ nullptr,
                                                  /* aVendorModel */ nullptr,
                                                  /* aVendorSwVersion */ nullptr,
                                                  /* aVendorData */ nullptr,
                                                  /* aCallback */ nullptr,
                                                  /* aContext */ nullptr));

    nexus.AdvanceTime(8 * Time::kOneSecondInMsec);

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        Coap::Message           *message = AsCoapMessagePtr(recvContext[i].mRelayRxMsgs.GetHead());
        Ip6::InterfaceIdentifier readIid;
        uint16_t                 joinerRouterRloc;

        if (i != 0)
        {
            VerifyOrQuit(message == nullptr);
            continue;
        }

        VerifyOrQuit(message != nullptr);

        VerifyOrQuit(message->ReadType() == Coap::kTypeNonConfirmable);
        VerifyOrQuit(message->ReadCode() == Coap::kCodePost);
        SuccessOrQuit(Tlv::Find<MeshCoP::JoinerIidTlv>(*message, readIid));
        SuccessOrQuit(Tlv::Find<MeshCoP::JoinerRouterLocatorTlv>(*message, joinerRouterRloc));

        VerifyOrQuit(readIid == joinerIids[1]);
        VerifyOrQuit(joinerRouterRloc = admitter.Get<Mle::Mle>().GetRloc16());
    }

    joiners[1]->Get<Joiner>().Stop();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send an `EnrollerJoinerAccept` message again accepting `joiners[1]` from `enrollers[0]`");

    message = enrollers[0]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerJoinerAccept);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(Tlv::Append<MeshCoP::JoinerIidTlv>(*message, joinerIids[1]));

    responseContexts[0].Clear();
    SuccessOrQuit(enrollers[0]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[0]));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(responseContexts[0].mReceived);
    VerifyOrQuit(responseContexts[0].mResponseState == MeshCoP::StateTlv::kAccept);
    VerifyOrQuit(!responseContexts[0].mHasAdmitterState);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that there is no change in the `enrollers` list and the tracked `joiners` on `admitter`");

    foundEnrollers.Clear();
    iter.Init(admitter.GetInstance());

    while (iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNone)
    {
        uint8_t matchedIndex;

        LogEnroller(enrollerInfo);

        matchedIndex = FindMatchingEnroller<kNumEnrollers>(enrollerInfo, kEnrollerIds, foundEnrollers);

        VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
        VerifyOrQuit(enrollerInfo.mMode == modes[matchedIndex]);

        if (matchedIndex == 0)
        {
            uint16_t numJoiners = 0;

            foundJoiners.Clear();

            while (iter.GetNextJoinerInfo(joinerInfo) == kErrorNone)
            {
                LogJoiner(joinerInfo);

                numJoiners++;

                for (uint8_t j = 0; j < 2; j++)
                {
                    if (joinerIids[j] == AsCoreType(&joinerInfo.mIid))
                    {
                        VerifyOrQuit(!foundJoiners.Has(j));
                        foundJoiners.Add(j);
                    }
                }
            }

            VerifyOrQuit(numJoiners == 2);
        }
        else
        {
            VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);
        }

        VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send an `EnrollerJoinerRelease` message from `enrollers[0]` to `admitter` releasing `joiners[0]`");

    message = enrollers[0]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerJoinerRelease);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(Tlv::Append<MeshCoP::JoinerIidTlv>(*message, joinerIids[0]));

    responseContexts[0].Clear();
    SuccessOrQuit(enrollers[0]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[0]));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(responseContexts[0].mReceived);
    VerifyOrQuit(responseContexts[0].mResponseState == MeshCoP::StateTlv::kAccept);
    VerifyOrQuit(!responseContexts[0].mHasAdmitterState);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that the released `joiners[0]` is removed on `admitter`");

    foundEnrollers.Clear();
    iter.Init(admitter.GetInstance());

    while (iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNone)
    {
        uint8_t matchedIndex;

        LogEnroller(enrollerInfo);

        matchedIndex = FindMatchingEnroller<kNumEnrollers>(enrollerInfo, kEnrollerIds, foundEnrollers);

        VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
        VerifyOrQuit(enrollerInfo.mMode == modes[matchedIndex]);

        if (matchedIndex == 0)
        {
            SuccessOrQuit(iter.GetNextJoinerInfo(joinerInfo));
            VerifyOrQuit(AsCoreType(&joinerInfo.mIid) == joinerIids[1]);
            LogJoiner(joinerInfo);
        }

        VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);
    }

    VerifyOrQuit(DidFindAllEnrollers<kNumEnrollers>(foundEnrollers));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send an `EnrollerJoinerRelease` message again releasing `joiners[0]` from `enrollers[0]`");

    message = enrollers[0]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerJoinerRelease);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(Tlv::Append<MeshCoP::JoinerIidTlv>(*message, joinerIids[0]));

    responseContexts[0].Clear();
    SuccessOrQuit(enrollers[0]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[0]));

    Log("Validate that `EnrollerJoinerRelease` is accepted, even though the given IID is already removed");

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(responseContexts[0].mReceived);
    VerifyOrQuit(responseContexts[0].mResponseState == MeshCoP::StateTlv::kAccept);
    VerifyOrQuit(!responseContexts[0].mHasAdmitterState);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send an `EnrollerJoinerRelease` message releasing `joiners[1]` from `enrollers[0]`");

    message = enrollers[0]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerJoinerRelease);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(Tlv::Append<MeshCoP::JoinerIidTlv>(*message, joinerIids[1]));

    responseContexts[0].Clear();
    SuccessOrQuit(enrollers[0]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[0]));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(responseContexts[0].mReceived);
    VerifyOrQuit(responseContexts[0].mResponseState == MeshCoP::StateTlv::kAccept);
    VerifyOrQuit(!responseContexts[0].mHasAdmitterState);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send two `EnrollerJoinerAccept` messages from `enrollers[2]` accepting both `joiners`");

    for (uint8_t j = 0; j < 2; j++)
    {
        message = enrollers[2]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerJoinerAccept);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::JoinerIidTlv>(*message, joinerIids[j]));

        responseContexts[2].Clear();
        SuccessOrQuit(
            enrollers[2]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[2]));

        nexus.AdvanceTime(Time::kOneSecondInMsec);

        VerifyOrQuit(responseContexts[2].mReceived);
        VerifyOrQuit(responseContexts[2].mResponseState == MeshCoP::StateTlv::kAccept);
        VerifyOrQuit(!responseContexts[2].mHasAdmitterState);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that both accepted joiners are tracked by `enrollers[2]` on `admitter`");

    foundEnrollers.Clear();
    iter.Init(admitter.GetInstance());

    while (iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNone)
    {
        uint8_t matchedIndex;

        LogEnroller(enrollerInfo);

        matchedIndex = FindMatchingEnroller<kNumEnrollers>(enrollerInfo, kEnrollerIds, foundEnrollers);

        VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
        VerifyOrQuit(enrollerInfo.mMode == modes[matchedIndex]);

        if (matchedIndex == 2)
        {
            uint16_t numJoiners = 0;

            foundJoiners.Clear();

            while (iter.GetNextJoinerInfo(joinerInfo) == kErrorNone)
            {
                LogJoiner(joinerInfo);

                numJoiners++;

                for (uint8_t j = 0; j < 2; j++)
                {
                    if (joinerIids[j] == AsCoreType(&joinerInfo.mIid))
                    {
                        VerifyOrQuit(!foundJoiners.Has(j));
                        foundJoiners.Add(j);
                    }
                }
            }

            VerifyOrQuit(numJoiners == 2);
        }
        else
        {
            VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);
        }
    }

    VerifyOrQuit(DidFindAllEnrollers<kNumEnrollers>(foundEnrollers));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send an `EnrollerJoinerRelease` message from `enrollers[2]` with wildcard IID releasing all joiners");

    message = enrollers[2]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerJoinerRelease);
    VerifyOrQuit(message != nullptr);

    wildcardJoinerIid.Clear();
    SuccessOrQuit(Tlv::Append<MeshCoP::JoinerIidTlv>(*message, wildcardJoinerIid));

    responseContexts[2].Clear();
    SuccessOrQuit(enrollers[2]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[2]));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(responseContexts[2].mReceived);
    VerifyOrQuit(responseContexts[2].mResponseState == MeshCoP::StateTlv::kAccept);
    VerifyOrQuit(!responseContexts[2].mHasAdmitterState);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that all previously accepted joiners by `enrollers[2]` on `admitter` are now removed");

    foundEnrollers.Clear();
    iter.Init(admitter.GetInstance());

    while (iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNone)
    {
        uint8_t matchedIndex;

        LogEnroller(enrollerInfo);

        matchedIndex = FindMatchingEnroller<kNumEnrollers>(enrollerInfo, kEnrollerIds, foundEnrollers);

        VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
        VerifyOrQuit(enrollerInfo.mMode == modes[matchedIndex]);

        VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send an invalid `EnrollerJoinerAccept` message from enrollers[2] with wildcard IID");

    message = enrollers[2]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerJoinerAccept);
    VerifyOrQuit(message != nullptr);

    wildcardJoinerIid.Clear();
    SuccessOrQuit(Tlv::Append<MeshCoP::JoinerIidTlv>(*message, wildcardJoinerIid));

    responseContexts[2].Clear();
    SuccessOrQuit(enrollers[2]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[2]));

    Log("Validate that the invalid `EnrollerJoinerAccept` is correctly rejected");

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(responseContexts[2].mReceived);
    VerifyOrQuit(responseContexts[2].mResponseState == MeshCoP::StateTlv::kReject);
    VerifyOrQuit(!responseContexts[2].mHasAdmitterState);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send two `EnrollerJoinerAccept` messages from `enrollers[2]` accepting both `joiners`");

    for (uint8_t j = 0; j < 2; j++)
    {
        message = enrollers[2]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerJoinerAccept);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::JoinerIidTlv>(*message, joinerIids[j]));

        responseContexts[2].Clear();
        SuccessOrQuit(
            enrollers[2]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[2]));

        nexus.AdvanceTime(Time::kOneSecondInMsec);

        VerifyOrQuit(responseContexts[2].mReceived);
        VerifyOrQuit(responseContexts[2].mResponseState == MeshCoP::StateTlv::kAccept);
        VerifyOrQuit(!responseContexts[2].mHasAdmitterState);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that both accepted Joiners are tracked by `enrollers[2]` on `admitter`");

    foundEnrollers.Clear();
    iter.Init(admitter.GetInstance());

    while (iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNone)
    {
        uint8_t matchedIndex;

        LogEnroller(enrollerInfo);

        matchedIndex = FindMatchingEnroller<kNumEnrollers>(enrollerInfo, kEnrollerIds, foundEnrollers);

        VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
        VerifyOrQuit(enrollerInfo.mMode == modes[matchedIndex]);

        if (matchedIndex == 2)
        {
            uint16_t numJoiners = 0;

            foundJoiners.Clear();

            while (iter.GetNextJoinerInfo(joinerInfo) == kErrorNone)
            {
                LogJoiner(joinerInfo);

                numJoiners++;

                for (uint8_t j = 0; j < 2; j++)
                {
                    if (joinerIids[j] == AsCoreType(&joinerInfo.mIid))
                    {
                        VerifyOrQuit(!foundJoiners.Has(j));
                        foundJoiners.Add(j);
                    }
                }

                VerifyOrQuit(joinerInfo.mMsecTillExpiration >= 6 * Time::kOneMinuteInMsec);
            }

            VerifyOrQuit(numJoiners == 2);
        }
        else
        {
            VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);
        }
    }

    VerifyOrQuit(DidFindAllEnrollers<kNumEnrollers>(foundEnrollers));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Wait for 4 minutes, sending `EnrollerKeepAlive` every 30 seconds to maintain enroller connections");

    for (uint8_t interval = 0; interval < 4 * 2; interval++)
    {
        Log("Send `EnrollerKeepAlive` message from all `enrollers` to maintain the connection");

        for (uint8_t i = 0; i < kNumEnrollers; i++)
        {
            message = enrollers[i]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerKeepAlive);
            VerifyOrQuit(message != nullptr);

            SuccessOrQuit(Tlv::Append<MeshCoP::StateTlv>(*message, MeshCoP::StateTlv::kAccept));

            responseContexts[i].Clear();
            SuccessOrQuit(
                enrollers[i]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[i]));
        }

        nexus.AdvanceTime(Time::kOneSecondInMsec);

        for (uint8_t i = 0; i < kNumEnrollers; i++)
        {
            VerifyOrQuit(responseContexts[i].mReceived);
            VerifyOrQuit(responseContexts[i].mResponseState == MeshCoP::StateTlv::kAccept);
            VerifyOrQuit(responseContexts[i].mHasAdmitterState);
        }

        nexus.AdvanceTime(29 * Time::kOneSecondInMsec);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate the enroller list on `admitter` and that both joiners are still accepted by `enrollers[2]`");

    foundEnrollers.Clear();
    iter.Init(admitter.GetInstance());

    while (iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNone)
    {
        uint8_t matchedIndex;

        LogEnroller(enrollerInfo);

        matchedIndex = FindMatchingEnroller<kNumEnrollers>(enrollerInfo, kEnrollerIds, foundEnrollers);

        VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
        VerifyOrQuit(enrollerInfo.mMode == modes[matchedIndex]);

        if (matchedIndex == 2)
        {
            uint16_t numJoiners = 0;

            foundJoiners.Clear();

            while (iter.GetNextJoinerInfo(joinerInfo) == kErrorNone)
            {
                LogJoiner(joinerInfo);

                numJoiners++;

                for (uint8_t j = 0; j < 2; j++)
                {
                    if (joinerIids[j] == AsCoreType(&joinerInfo.mIid))
                    {
                        VerifyOrQuit(!foundJoiners.Has(j));
                        foundJoiners.Add(j);
                    }
                }

                // Since we waited for 4 minutes, the joiner expiration time should be closer

                VerifyOrQuit(joinerInfo.mMsecTillExpiration < 6 * Time::kOneMinuteInMsec);
            }

            VerifyOrQuit(numJoiners == 2);
        }
        else
        {
            VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);
        }
    }

    VerifyOrQuit(DidFindAllEnrollers<kNumEnrollers>(foundEnrollers));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Start `joiners[1]` and validate that its RelayRx are only forwarded to `enrollers[2]`");

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        recvContext[i].Clear();
    }

    joiners[1]->Get<ThreadNetif>().Up();
    SuccessOrQuit(joiners[1]->Get<Joiner>().Start(kPskd,
                                                  /* aProvisioningUrl */ nullptr,
                                                  /* aVendorName */ nullptr,
                                                  /* aVendorModel */ nullptr,
                                                  /* aVendorSwVersion */ nullptr,
                                                  /* aVendorData */ nullptr,
                                                  /* aCallback */ nullptr,
                                                  /* aContext */ nullptr));

    nexus.AdvanceTime(8 * Time::kOneSecondInMsec);

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        Coap::Message           *message = AsCoapMessagePtr(recvContext[i].mRelayRxMsgs.GetHead());
        Ip6::InterfaceIdentifier readIid;
        uint16_t                 joinerRouterRloc;

        if (i != 2)
        {
            VerifyOrQuit(message == nullptr);
            continue;
        }

        VerifyOrQuit(message != nullptr);

        VerifyOrQuit(message->ReadType() == Coap::kTypeNonConfirmable);
        VerifyOrQuit(message->ReadCode() == Coap::kCodePost);
        SuccessOrQuit(Tlv::Find<MeshCoP::JoinerIidTlv>(*message, readIid));
        SuccessOrQuit(Tlv::Find<MeshCoP::JoinerRouterLocatorTlv>(*message, joinerRouterRloc));

        VerifyOrQuit(readIid == joinerIids[1]);
        VerifyOrQuit(joinerRouterRloc = admitter.Get<Mle::Mle>().GetRloc16());
    }

    joiners[1]->Get<Joiner>().Stop();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate expiration is extended for joiners[1] after it transmitted");

    foundEnrollers.Clear();
    iter.Init(admitter.GetInstance());

    while (iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNone)
    {
        uint8_t matchedIndex;

        LogEnroller(enrollerInfo);

        matchedIndex = FindMatchingEnroller<kNumEnrollers>(enrollerInfo, kEnrollerIds, foundEnrollers);

        VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
        VerifyOrQuit(enrollerInfo.mMode == modes[matchedIndex]);

        if (matchedIndex == 2)
        {
            uint16_t numJoiners = 0;

            foundJoiners.Clear();

            while (iter.GetNextJoinerInfo(joinerInfo) == kErrorNone)
            {
                LogJoiner(joinerInfo);

                numJoiners++;

                for (uint8_t j = 0; j < 2; j++)
                {
                    if (joinerIids[j] == AsCoreType(&joinerInfo.mIid))
                    {
                        VerifyOrQuit(!foundJoiners.Has(j));
                        foundJoiners.Add(j);

                        // `joiners[0]` expiration should still tick down, while `joiners[1]`'s should be extended

                        if (j == 0)
                        {
                            VerifyOrQuit(joinerInfo.mMsecTillExpiration < 6 * Time::kOneMinuteInMsec);
                        }
                        else
                        {
                            VerifyOrQuit(joinerInfo.mMsecTillExpiration >= 6 * Time::kOneMinuteInMsec);
                        }
                    }
                }
            }

            VerifyOrQuit(numJoiners == 2);
        }
        else
        {
            VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);
        }
    }

    VerifyOrQuit(DidFindAllEnrollers<kNumEnrollers>(foundEnrollers));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Wait for another 4 minutes, sending `EnrollerKeepAlive` every 30 seconds");

    for (uint8_t interval = 0; interval < 4 * 2; interval++)
    {
        Log("Send `EnrollerKeepAlive` message from all `enrollers` to maintain the connection");

        for (uint8_t i = 0; i < kNumEnrollers; i++)
        {
            message = enrollers[i]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerKeepAlive);
            VerifyOrQuit(message != nullptr);

            SuccessOrQuit(Tlv::Append<MeshCoP::StateTlv>(*message, MeshCoP::StateTlv::kAccept));

            responseContexts[i].Clear();
            SuccessOrQuit(
                enrollers[i]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[i]));
        }

        nexus.AdvanceTime(Time::kOneSecondInMsec);

        for (uint8_t i = 0; i < kNumEnrollers; i++)
        {
            VerifyOrQuit(responseContexts[i].mReceived);
            VerifyOrQuit(responseContexts[i].mResponseState == MeshCoP::StateTlv::kAccept);
            VerifyOrQuit(responseContexts[i].mHasAdmitterState);
        }

        nexus.AdvanceTime(29 * Time::kOneSecondInMsec);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that `joiners[0]` is timed out and removed on `admitter`");

    foundEnrollers.Clear();
    iter.Init(admitter.GetInstance());

    while (iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNone)
    {
        uint8_t matchedIndex;

        LogEnroller(enrollerInfo);

        matchedIndex = FindMatchingEnroller<kNumEnrollers>(enrollerInfo, kEnrollerIds, foundEnrollers);

        VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
        VerifyOrQuit(enrollerInfo.mMode == modes[matchedIndex]);

        if (matchedIndex == 2)
        {
            SuccessOrQuit(iter.GetNextJoinerInfo(joinerInfo));

            LogJoiner(joinerInfo);

            VerifyOrQuit(joinerIids[1] == AsCoreType(&joinerInfo.mIid));
            VerifyOrQuit(joinerInfo.mMsecTillExpiration > 0);
        }

        VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Wait for another 4 minutes, sending `EnrollerKeepAlive` every 30 seconds");

    for (uint8_t interval = 0; interval < 4 * 2; interval++)
    {
        Log("Send `EnrollerKeepAlive` message from all `enrollers` to maintain the connection");

        for (uint8_t i = 0; i < kNumEnrollers; i++)
        {
            message = enrollers[i]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerKeepAlive);
            VerifyOrQuit(message != nullptr);

            SuccessOrQuit(Tlv::Append<MeshCoP::StateTlv>(*message, MeshCoP::StateTlv::kAccept));

            responseContexts[i].Clear();
            SuccessOrQuit(
                enrollers[i]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[i]));
        }

        nexus.AdvanceTime(Time::kOneSecondInMsec);

        for (uint8_t i = 0; i < kNumEnrollers; i++)
        {
            VerifyOrQuit(responseContexts[i].mReceived);
            VerifyOrQuit(responseContexts[i].mResponseState == MeshCoP::StateTlv::kAccept);
            VerifyOrQuit(responseContexts[i].mHasAdmitterState);
        }

        nexus.AdvanceTime(29 * Time::kOneSecondInMsec);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that `joiners[1]` is also timed out and removed on `admitter`");

    foundEnrollers.Clear();
    iter.Init(admitter.GetInstance());

    while (iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNone)
    {
        uint8_t matchedIndex;

        LogEnroller(enrollerInfo);

        matchedIndex = FindMatchingEnroller<kNumEnrollers>(enrollerInfo, kEnrollerIds, foundEnrollers);

        VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
        VerifyOrQuit(enrollerInfo.mMode == modes[matchedIndex]);
        VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);
    }

    VerifyOrQuit(DidFindAllEnrollers<kNumEnrollers>(foundEnrollers));
}

void TestBorderAdmitterForwardingUdpProxy(void)
{
    static constexpr uint8_t kNumEnrollers = 4;

    static const char *kEnrollerIds[kNumEnrollers] = {"1", "2", "3", "4"};

    static const uint8_t kDiagTlvs[] = {NetworkDiagnostic::Tlv::kExtMacAddress, NetworkDiagnostic::Tlv::kVersion};

    Core                               nexus;
    Node                              &admitter = nexus.CreateNode();
    Node                              *enrollers[kNumEnrollers];
    Ip6::SockAddr                      sockAddr;
    Pskc                               pskc;
    Coap::Message                     *message;
    Coap::Message                     *diagMessage;
    uint8_t                            modes[kNumEnrollers];
    ResponseContext                    responseContexts[kNumEnrollers];
    ReceiveContext                     recvContext[kNumEnrollers];
    MeshCoP::SteeringData              steeringData;
    MeshCoP::SteeringData              leaderSteeringData;
    Admitter::Iterator                 iter;
    Admitter::EnrollerInfo             enrollerInfo;
    Admitter::JoinerInfo               joinerInfo;
    BitSet<kNumEnrollers>              foundEnrollers;
    uint16_t                           sessionId;
    uint16_t                           rloc16;
    MeshCoP::UdpEncapsulationTlvHeader udpEncapHeader;
    ExtendedTlv                        extTlv;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestBorderAdmitterForwardingUdpProxy");

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        enrollers[i] = &nexus.CreateNode();
    }

    nexus.AdvanceTime(0);

    // Form the topology:
    // - `admitter` forms the network (as leader)
    // - All enrollers stay disconnected.

    admitter.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Mle::Mle>().IsLeader());

    for (Node *enroller : enrollers)
    {
        SuccessOrQuit(enroller->Get<Mac::Mac>().SetPanChannel(admitter.Get<Mac::Mac>().GetPanChannel()));
        enroller->Get<Mac::Mac>().SetPanId(admitter.Get<Mac::Mac>().GetPanId());
        enroller->Get<ThreadNetif>().Up();
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Enable Border Admitter on `admitter`");

    admitter.Get<Admitter>().SetEnabled(true);
    VerifyOrQuit(admitter.Get<Admitter>().IsEnabled());
    VerifyOrQuit(!admitter.Get<Admitter>().IsPrimeAdmitter());

    nexus.AdvanceTime(45 * Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(!admitter.Get<Admitter>().IsActiveCommissioner());

    SuccessOrQuit(admitter.Get<Ip6::Filter>().AddUnsecurePort(admitter.Get<Manager>().GetUdpPort()));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Establish a DTLS connection from all `enrollers` to `admitter`");

    sockAddr.SetAddress(admitter.Get<Mle::Mle>().GetLinkLocalAddress());
    sockAddr.SetPort(admitter.Get<Manager>().GetUdpPort());

    admitter.Get<KeyManager>().GetPskc(pskc);

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        Node *enroller = enrollers[i];

        SuccessOrQuit(enroller->Get<Tmf::SecureAgent>().SetPsk(pskc.m8, Pskc::kSize));

        recvContext[i].Clear();
        enroller->Get<Tmf::SecureAgent>().RegisterResourceHandler(HandleResource, &recvContext[i]);

        SuccessOrQuit(enroller->Get<Tmf::SecureAgent>().Open());
        SuccessOrQuit(enroller->Get<Tmf::SecureAgent>().Connect(sockAddr));

        nexus.AdvanceTime(Time::kOneSecondInMsec);

        VerifyOrQuit(enroller->Get<Tmf::SecureAgent>().IsConnected());
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Prepare mode for each enroller");

    modes[0] = MeshCoP::EnrollerModeTlv::kForwardJoinerRelayRx | MeshCoP::EnrollerModeTlv::kForwardUdpProxyRx;
    modes[1] = MeshCoP::EnrollerModeTlv::kForwardJoinerRelayRx | MeshCoP::EnrollerModeTlv::kForwardUdpProxyRx;
    modes[2] = MeshCoP::EnrollerModeTlv::kForwardUdpProxyRx;
    modes[3] = MeshCoP::EnrollerModeTlv::kForwardJoinerRelayRx;

    steeringData.SetToPermitAllJoiners();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Send an `EnrollerRegister` message from all `enrollers`");

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        message = enrollers[i]->Get<Tmf::SecureAgent>().NewPriorityConfirmablePostMessage(kUriEnrollerRegister);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerIdTlv>(*message, kEnrollerIds[i]));
        SuccessOrQuit(Tlv::Append<MeshCoP::EnrollerModeTlv>(*message, modes[i]));
        SuccessOrQuit(
            Tlv::Append<MeshCoP::SteeringDataTlv>(*message, steeringData.GetData(), steeringData.GetLength()));

        responseContexts[i].Clear();
        SuccessOrQuit(
            enrollers[i]->Get<Tmf::SecureAgent>().SendMessage(*message, HandleResponse, &responseContexts[i]));
    }

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check that all registrations were accepted");

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        VerifyOrQuit(responseContexts[i].mReceived);
        VerifyOrQuit(responseContexts[i].mResponseState == MeshCoP::StateTlv::kAccept);
        VerifyOrQuit(responseContexts[i].mHasAdmitterState);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that `admitter` becomes active commissioner");

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    VerifyOrQuit(admitter.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(admitter.Get<Admitter>().IsActiveCommissioner());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindBorderAgentRloc(rloc16));
    VerifyOrQuit(rloc16 == admitter.Get<Mle::Mle>().GetRloc16());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindCommissioningSessionId(sessionId));
    VerifyOrQuit(sessionId == admitter.Get<Admitter>().GetCommissionerSessionId());

    SuccessOrQuit(admitter.Get<NetworkData::Leader>().FindSteeringData(leaderSteeringData));
    VerifyOrQuit(leaderSteeringData == steeringData);
    VerifyOrQuit(leaderSteeringData.PermitsAllJoiners());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate the Enroller info on `admitter`");

    foundEnrollers.Clear();
    iter.Init(admitter.GetInstance());

    while (iter.GetNextEnrollerInfo(enrollerInfo) == kErrorNone)
    {
        uint8_t matchedIndex;

        LogEnroller(enrollerInfo);

        matchedIndex = FindMatchingEnroller<kNumEnrollers>(enrollerInfo, kEnrollerIds, foundEnrollers);

        VerifyOrQuit(AsCoreType(&enrollerInfo.mSteeringData) == steeringData);
        VerifyOrQuit(enrollerInfo.mMode == modes[matchedIndex]);

        VerifyOrQuit(iter.GetNextJoinerInfo(joinerInfo) == kErrorNotFound);
    }

    VerifyOrQuit(DidFindAllEnrollers<kNumEnrollers>(foundEnrollers));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Prepare a `DiagnosticGetQuery` message");

    diagMessage = enrollers[0]->Get<Tmf::Agent>().NewNonConfirmablePostMessage(kUriDiagnosticGetQuery);
    VerifyOrQuit(diagMessage != nullptr);
    SuccessOrQuit(Tlv::Append<NetworkDiagnostic::TypeListTlv>(*diagMessage, kDiagTlvs, sizeof(kDiagTlvs)));
    diagMessage->WriteMessageId(0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Embed the `DiagnosticGetQuery` into `ProxyTx` message and send it from `enrollers[0]`");

    message = enrollers[0]->Get<Tmf::SecureAgent>().NewPriorityNonConfirmablePostMessage(kUriProxyTx);
    VerifyOrQuit(message != nullptr);

    udpEncapHeader.SetSourcePort(Tmf::kUdpPort);
    udpEncapHeader.SetDestinationPort(Tmf::kUdpPort);

    extTlv.SetType(MeshCoP::Tlv::kUdpEncapsulation);
    extTlv.SetLength(sizeof(udpEncapHeader) + diagMessage->GetLength());

    SuccessOrQuit(message->Append(extTlv));
    SuccessOrQuit(message->Append(udpEncapHeader));
    SuccessOrQuit(message->AppendBytesFromMessage(*diagMessage, 0, diagMessage->GetLength()));
    diagMessage->Free();

    SuccessOrQuit(Tlv::Append<MeshCoP::Ip6AddressTlv>(*message, admitter.Get<Mle::Mle>().GetMeshLocalRloc()));

    SuccessOrQuit(enrollers[0]->Get<Tmf::SecureAgent>().SendMessage(*message));

    nexus.AdvanceTime(Time::kOneSecondInMsec);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate that `admitter` receives the `DiagnosticGetQuery` response");
    Log("And that it forwards it as `ProxyRx` messages to all `enrollers` with `kForwardUdpProxyRx` mode flag");

    for (uint8_t i = 0; i < kNumEnrollers; i++)
    {
        Coap::Message *message = AsCoapMessagePtr(recvContext[i].mProxyRxMsgs.GetHead());
        Ip6::Address   senderAddr;
        OffsetRange    offsetRange;

        if ((modes[i] & MeshCoP::EnrollerModeTlv::kForwardUdpProxyRx) == 0)
        {
            VerifyOrQuit(message == nullptr);
            Log("   Enroller %s does not set `kForwardUdpProxyRx` mode - so did not get `ProxyRx`", kEnrollerIds[i]);
            continue;
        }

        VerifyOrQuit(message != nullptr);

        VerifyOrQuit(message->ReadType() == Coap::kTypeNonConfirmable);
        VerifyOrQuit(message->ReadCode() == Coap::kCodePost);

        SuccessOrQuit(Tlv::FindTlvValueOffsetRange(*message, MeshCoP::Tlv::kUdpEncapsulation, offsetRange));

        SuccessOrQuit(Tlv::Find<MeshCoP::Ip6AddressTlv>(*message, senderAddr));
        VerifyOrQuit(senderAddr == admitter.Get<Mle::Mle>().GetMeshLocalRloc());

        Log("   Enroller %s received `ProxyRx` from %s", kEnrollerIds[i], senderAddr.ToString().AsCString());
    }
}

//---------------------------------------------------------------------------------------------------------------------

static constexpr uint32_t kInfraIfIndex = 1;

void ValidateAdmitterMdnsService(Node &aNode)
{
    static const char kDefaultServiceBaseName[] = OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_BASE_NAME;

    Dns::Multicast::Core::Iterator  *iterator;
    Dns::Multicast::Core::Service    service;
    Dns::Multicast::Core::EntryState entryState;

    iterator = aNode.Get<Dns::Multicast::Core>().AllocateIterator();
    VerifyOrQuit(iterator != nullptr);

    SuccessOrQuit(aNode.Get<Dns::Multicast::Core>().GetNextService(*iterator, service, entryState));

    Log("  HostName: %s", service.mHostName);
    Log("  ServiceInstance: %s", service.mServiceInstance);
    Log("  ServiceType: %s", service.mServiceType);

    for (uint16_t i = 0; i < service.mSubTypeLabelsLength; i++)
    {
        Log("  SubType: %s", service.mSubTypeLabels[i]);
    }

    Log("  Port: %u", service.mPort);
    Log("  TTL: %lu", ToUlong(service.mTtl));

    VerifyOrQuit(StringMatch(service.mServiceType, "_meshcop._udp"));
    VerifyOrQuit(StringStartsWith(service.mServiceInstance, kDefaultServiceBaseName));
    VerifyOrQuit(StringStartsWith(service.mHostName, "ot"));
    VerifyOrQuit(service.mPort == aNode.Get<MeshCoP::BorderAgent::Manager>().GetUdpPort());
    VerifyOrQuit(service.mTtl > 0);
    VerifyOrQuit(service.mInfraIfIndex == 1);
    VerifyOrQuit(entryState == OT_MDNS_ENTRY_STATE_REGISTERED);

    if (aNode.Get<Admitter>().IsPrimeAdmitter())
    {
        VerifyOrQuit(service.mSubTypeLabelsLength == 1);
        VerifyOrQuit(StringMatch(service.mSubTypeLabels[0], "_admitter"));
    }
    else
    {
        VerifyOrQuit(service.mSubTypeLabelsLength == 0);
    }

    VerifyOrQuit(aNode.Get<Dns::Multicast::Core>().GetNextService(*iterator, service, entryState) == kErrorNotFound);

    aNode.Get<Dns::Multicast::Core>().FreeIterator(*iterator);
}

//---------------------------------------------------------------------------------------------------------------------

void TestBorderAdmitterDnssdService(void)
{
    Core                             nexus;
    Node                            &node1 = nexus.CreateNode();
    Node                            &node2 = nexus.CreateNode();
    Dns::Multicast::Core::Iterator  *iterator;
    Dns::Multicast::Core::Service    service;
    Dns::Multicast::Core::EntryState entryState;

    Log("------------------------------------------------------------------------------------------------------");
    Log("TestBorderAdmitterDnssdService");

    nexus.AdvanceTime(0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Enable mDNS
    SuccessOrQuit(node1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    VerifyOrQuit(node1.Get<Dns::Multicast::Core>().IsEnabled());
    SuccessOrQuit(node2.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    VerifyOrQuit(node2.Get<Dns::Multicast::Core>().IsEnabled());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Form the topology.

    node1.Form();
    nexus.AdvanceTime(50 * Time::kOneSecondInMsec);
    node2.Join(node1);

    nexus.AdvanceTime(10 * Time::kOneMinuteInMsec);

    VerifyOrQuit(node1.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(node2.Get<Mle::Mle>().IsRouter());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check Border Admitter initial state");

    VerifyOrQuit(!node1.Get<Admitter>().IsEnabled());
    VerifyOrQuit(!node2.Get<Admitter>().IsEnabled());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Enable Admitter role on `node1` and validate that it becomes the Prime Admitter");

    node1.Get<Admitter>().SetEnabled(true);
    VerifyOrQuit(node1.Get<Admitter>().IsEnabled());

    nexus.AdvanceTime(45 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(!node1.Get<Admitter>().IsActiveCommissioner());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate the registered mDNS MeshCop service by `node1` including `_admitter` sub-type");

    ValidateAdmitterMdnsService(node1);
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate the registered mDNS MeshCop service by `node2` (should not have `_admitter` sub-type)");

    ValidateAdmitterMdnsService(node2);
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Enable Admitter role on `node2` and validate that `node1` remains the Prime Admitter");

    node2.Get<Admitter>().SetEnabled(true);
    VerifyOrQuit(node2.Get<Admitter>().IsEnabled());

    nexus.AdvanceTime(45 * Time::kOneSecondInMsec);

    VerifyOrQuit(node1.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(!node1.Get<Admitter>().IsActiveCommissioner());
    VerifyOrQuit(!node2.Get<Admitter>().IsPrimeAdmitter());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate the registered mDNS MeshCop service by `node1` including `_admitter` sub-type");

    ValidateAdmitterMdnsService(node1);
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate the registered mDNS MeshCop service by `node2` (should not have `_admitter` sub-type)");
    ValidateAdmitterMdnsService(node2);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Disable Admitter role on `node1` and check that `node2` becomes the Prime Admitter");

    node1.Get<Admitter>().SetEnabled(false);
    VerifyOrQuit(!node1.Get<Admitter>().IsEnabled());
    VerifyOrQuit(!node1.Get<Admitter>().IsPrimeAdmitter());

    nexus.AdvanceTime(75 * Time::kOneSecondInMsec);

    VerifyOrQuit(node2.Get<Admitter>().IsPrimeAdmitter());
    VerifyOrQuit(!node2.Get<Admitter>().IsActiveCommissioner());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate the registered mDNS MeshCop service by `node1` (no longer publishing `_admitter` sub-type)");

    ValidateAdmitterMdnsService(node1);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Validate the registered mDNS MeshCop service by `node2` (now should include `_admitter` sub-type)");
    ValidateAdmitterMdnsService(node2);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestBorderAdmitterPrimeSelection();
    ot::Nexus::TestBorderAdmitterEnrollerInteraction();
    ot::Nexus::TestBorderAdmitterCommissionerConflictAndPetitionerRetry();
    ot::Nexus::TestBorderAdmitterMultipleEnrollers();
    ot::Nexus::TestBorderAdmitterJoinerEnrollerInteraction();
    ot::Nexus::TestBorderAdmitterForwardingUdpProxy();
    ot::Nexus::TestBorderAdmitterDnssdService();

    printf("\nAll tests passed\n");
    return 0;
}
