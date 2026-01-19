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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t time100Milliseconds = 100;
static constexpr uint32_t time1Second         = (1 * 1000);
static constexpr uint32_t time5Seconds        = 5 * time1Second;
static constexpr uint32_t time10Seconds       = 10 * time1Second;
static constexpr uint32_t time20Seconds       = 2 * time10Seconds;
static constexpr uint32_t time30Seconds       = 3 * time10Seconds;
static constexpr uint32_t time1Minute         = 60 * 1000;
static constexpr uint32_t time2Minutes        = 2 * time1Minute;
static constexpr uint32_t time5Minutes        = 5 * time1Minute;
static constexpr uint32_t time10Minutes       = 2 * time5Minutes;
static constexpr uint32_t time30Minutes       = 3 * time10Minutes;
static constexpr uint32_t time1Hour           = 6 * time10Minutes;
uint16_t                  numCallbackExecuted = 0;

void     enableCommissionerOnLeader(Core &aCore, Node &aLeader);
void     disableCommissionerOnLeader(Core &aCore, Node &aLeader);
void     setLogLevelAllNodes(Nexus::Core &aCore, LogLevel aLogLevel);
void     prepareForJoining(Node &aNode, Node::JoinMode aJoinMode);
uint32_t numCallbackExecutedMin(uint32_t aTimeout);
uint32_t numCallbackExecutedMax(uint32_t aTimeout);
void     joinerCallback(otError aResult, void *aContext);

void TestJoiner(void)
{
    Core nexus;
    Log("------------------------------- Test Joiner -------------------------------");
    Log("Join an FED, commissioner in normal operation, normal 'Single Shot' Joiner.");

    Node &leader        = nexus.CreateNode();
    Node &fed           = nexus.CreateNode();
    bool  fedIsJoined   = false;
    numCallbackExecuted = 0;

    setLogLevelAllNodes(nexus, kLogLevelInfo);
    nexus.AdvanceTime(0);

    leader.Form();
    nexus.AdvanceTime(time10Seconds + 3 * time1Second);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    enableCommissionerOnLeader(nexus, leader);

    prepareForJoining(fed, Node::kAsFed);
    nexus.AdvanceTime(time1Second);

    // dataset must not be commissioned to use joiner
    VerifyOrQuit(!fed.Get<MeshCoP::ActiveDatasetManager>().IsCommissioned());

    fed.Get<MeshCoP::Joiner>().Start("J01NRETRYTEST", nullptr, nullptr, nullptr, nullptr, nullptr, joinerCallback,
                                     &fedIsJoined);
    nexus.AdvanceTime(time10Seconds);
    VerifyOrQuit(fedIsJoined);
    fed.Get<Mle::Mle>().Start();
    nexus.AdvanceTime(time10Seconds);
    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(numCallbackExecuted == 1);

    Log("Test passed.");
    Log("Network was simulated for a duration of %lu seconds\n\n\n\n",
        ToUlong(Time::MsecToSec(nexus.GetNow().GetValue())));
}

void TestJoinerWithRetries(void)
{
    Core nexus;
    Log("------------------------------- Test Joiner With Retries -------------------------------");
    Log("Join an FED, commissioner in normal operation, Retrying Joiner.");

    Node &leader        = nexus.CreateNode();
    Node &fed           = nexus.CreateNode();
    bool  fedIsJoined   = false;
    numCallbackExecuted = 0;

    setLogLevelAllNodes(nexus, kLogLevelInfo);
    nexus.AdvanceTime(0);

    leader.Form();
    nexus.AdvanceTime(time10Seconds + 3 * time1Second);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    enableCommissionerOnLeader(nexus, leader);

    prepareForJoining(fed, Node::kAsFed);
    nexus.AdvanceTime(time1Second);

    // dataset must not be commissioned to use joiner
    VerifyOrQuit(!fed.Get<MeshCoP::ActiveDatasetManager>().IsCommissioned());

    fed.Get<MeshCoP::Joiner>().StartWithRetries("J01NRETRYTEST", nullptr, time1Second, time10Minutes, nullptr, nullptr,
                                                nullptr, nullptr, joinerCallback, &fedIsJoined);
    nexus.AdvanceTime(time10Seconds);
    VerifyOrQuit(fedIsJoined);
    fed.Get<Mle::Mle>().Start();
    nexus.AdvanceTime(time10Seconds);
    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(numCallbackExecuted == 1);

    Log("Test passed.");
    Log("Network was simulated for a duration of %lu seconds\n\n\n\n",
        ToUlong(Time::MsecToSec(nexus.GetNow().GetValue())));
}

void TestJoinerExecutesRetries(void)
{
    Core nexus;
    Log("------------------------------- Test Joiner Executes Retries -------------------------------");
    Log("Join an FED, Commissioner is Stopped, so Joiner must retry.");

    Node    &leader      = nexus.CreateNode();
    Node    &fed         = nexus.CreateNode();
    bool     fedIsJoined = false;
    uint32_t joinTimeout = time30Minutes;
    numCallbackExecuted  = 0;

    setLogLevelAllNodes(nexus, kLogLevelInfo);
    nexus.AdvanceTime(0);

    leader.Form();
    nexus.AdvanceTime(time10Seconds + 3 * time1Second);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    prepareForJoining(fed, Node::kAsFed);
    nexus.AdvanceTime(time1Second);

    VerifyOrQuit(!fed.Get<MeshCoP::ActiveDatasetManager>().IsCommissioned());

    fed.Get<MeshCoP::Joiner>().StartWithRetries("J01NRETRYTEST", nullptr, time1Second, joinTimeout, nullptr, nullptr,
                                                nullptr, nullptr, joinerCallback, &fedIsJoined);
    for (uint16_t i = 0; i < 190; i++)
    {
        // 190 * 10s = 1900s, a bit more than half an hour to ensure joiner timeout is reached
        nexus.AdvanceTime(time10Seconds);
        VerifyOrQuit(!fedIsJoined);
    }
    VerifyOrQuit(fed.Get<Mle::Mle>().IsDisabled());
    VerifyOrQuit(numCallbackExecuted >= numCallbackExecutedMin(joinTimeout));
    VerifyOrQuit(numCallbackExecuted <= numCallbackExecutedMax(joinTimeout));

    Log("Check logs to verify test was passed. Device must have retried joining for 30 minutes and failed. Delay "
        "Factor should not be larger than 16.");
    Log("Network was simulated for a duration of %lu seconds\n\n\n\n",
        ToUlong(Time::MsecToSec(nexus.GetNow().GetValue())));
}

void TestCommissionerStartsDuringRetryingJoiner(void)
{
    Core nexus;
    Log("----------------------------- Test Commissioner Starts During Retrying Joiner -----------------------------");
    Log("Join an FED, Commissioner is Started during retrying joiner is active. It should succeed to join.");

    Node    &leader          = nexus.CreateNode();
    Node    &fed             = nexus.CreateNode();
    bool     fedIsJoined     = false;
    uint32_t joinerStartTime = 0;
    numCallbackExecuted      = 0;

    TimeMilli commissionerStartTime;

    setLogLevelAllNodes(nexus, kLogLevelInfo);
    nexus.AdvanceTime(0);

    leader.Form();
    nexus.AdvanceTime(time10Seconds + 3 * time1Second);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    prepareForJoining(fed, Node::kAsFed);
    nexus.AdvanceTime(time1Second);

    VerifyOrQuit(!fed.Get<MeshCoP::ActiveDatasetManager>().IsCommissioned());

    // start commissioner 10 - 11 minutes after joiner
    commissionerStartTime = nexus.GetNow() + (time10Minutes + Random::NonCrypto::GetUint16());
    joinerStartTime       = nexus.GetNow().GetValue();
    fed.Get<MeshCoP::Joiner>().StartWithRetries("J01NRETRYTEST", nullptr, time1Second, time30Minutes, nullptr, nullptr,
                                                nullptr, nullptr, joinerCallback, &fedIsJoined);

    while (nexus.GetNow() < commissionerStartTime)
    {
        nexus.AdvanceTime(time100Milliseconds);
    }
    VerifyOrQuit(!fedIsJoined);

    enableCommissionerOnLeader(nexus, leader);

    nexus.AdvanceTime(time30Seconds);
    VerifyOrQuit(fedIsJoined);
    fed.Get<Mle::Mle>().Start();
    nexus.AdvanceTime(time10Seconds);
    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(numCallbackExecuted >= numCallbackExecutedMin(commissionerStartTime.GetValue() - joinerStartTime));
    VerifyOrQuit(numCallbackExecuted <= numCallbackExecutedMax(commissionerStartTime.GetValue() - joinerStartTime));

    Log("Check logs to verify test was passed. Device must have retried joining until commissioner was started, then "
        "succeeded to join and not retry anymore.");
    Log("Network was simulated for a duration of %lu seconds\n\n\n\n",
        ToUlong(Time::MsecToSec(nexus.GetNow().GetValue())));
}

void TestExecuteTwoJoiners(void)
{
    Core nexus;
    Log("------------------------------- Test Execute Two Joiners -------------------------------");
    Log("Start retrying joiner that fails, then start single-shot joiner and ensure that retry still works "
        "afterwards.");

    Error error;
    Node &leader        = nexus.CreateNode();
    Node &fed           = nexus.CreateNode();
    bool  fedIsJoined   = false;
    numCallbackExecuted = 0;

    setLogLevelAllNodes(nexus, kLogLevelInfo);
    nexus.AdvanceTime(0);

    leader.Form();
    nexus.AdvanceTime(time10Seconds + 3 * time1Second);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    prepareForJoining(fed, Node::kAsFed);
    nexus.AdvanceTime(time1Second);

    VerifyOrQuit(!fed.Get<MeshCoP::ActiveDatasetManager>().IsCommissioned());

    fed.Get<MeshCoP::Joiner>().StartWithRetries("J01NRETRYTEST", nullptr, time1Second, 2 * time10Minutes, nullptr,
                                                nullptr, nullptr, nullptr, joinerCallback, &fedIsJoined);
    nexus.AdvanceTime(time10Seconds);

    for (uint8_t i = 0; i < 20; i++)
    {
        Log("Starting single-shot Joiner attempt %u", i + 1);
        error = fed.Get<MeshCoP::Joiner>().Start("J01NRETRYTEST", nullptr, nullptr, nullptr, nullptr, nullptr,
                                                 joinerCallback, &fedIsJoined);
        if (error == OT_ERROR_BUSY)
        {
            Log("Single-shot Joiner attempt %u reported 'busy'", i + 1);
        }
        nexus.AdvanceTime(2 * time1Second);
        VerifyOrQuit(!fedIsJoined);
    }

    nexus.AdvanceTime(time1Minute);
    enableCommissionerOnLeader(nexus, leader);

    nexus.AdvanceTime(time30Seconds);
    VerifyOrQuit(fedIsJoined);
    fed.Get<Mle::Mle>().Start();
    nexus.AdvanceTime(time10Seconds);
    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());

    Log("Check logs to verify test was passed. Device must have retried joining until commissioner was started, then "
        "succeeded to join and not retry anymore.");
    Log("Network was simulated for a duration of %lu seconds\n\n\n\n",
        ToUlong(Time::MsecToSec(nexus.GetNow().GetValue())));
}

void TestStoppingJoiner(void)
{
    Core nexus;
    Log("------------------------------- Test Stopping Joiner in any State -------------------------------");
    Log("Starts and stops retrying joiner at various times.");

    Error error;
    Node &leader        = nexus.CreateNode();
    Node &fed           = nexus.CreateNode();
    bool  fedIsJoined   = false;
    numCallbackExecuted = 0;

    setLogLevelAllNodes(nexus, kLogLevelInfo);
    nexus.AdvanceTime(0);

    leader.Form();
    nexus.AdvanceTime(time10Seconds + 3 * time1Second);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    prepareForJoining(fed, Node::kAsFed);
    nexus.AdvanceTime(time1Second);

    VerifyOrQuit(!fed.Get<MeshCoP::ActiveDatasetManager>().IsCommissioned());

    for (uint8_t i = 0; i < 20; i++)
    {
        // Random timeouts ensure that joiner is stopped in different states
        error =
            fed.Get<MeshCoP::Joiner>().StartWithRetries("J01NRETRYTEST", nullptr, time1Second, time10Minutes, nullptr,
                                                        nullptr, nullptr, nullptr, joinerCallback, &fedIsJoined);
        VerifyOrQuit(error == OT_ERROR_NONE);
        // create random timeout between 1s and roughly 9s
        nexus.AdvanceTime(1 * time1Second + Random::NonCrypto::GetUint16() / 8);
        // roughly 6s after stop required, in case discover scan is still running
        fed.Get<MeshCoP::Joiner>().Stop();
        nexus.AdvanceTime(6 * time1Second + Random::NonCrypto::GetUint16() / 8);
    }

    nexus.AdvanceTime(time10Seconds);
    fed.Get<MeshCoP::Joiner>().StartWithRetries("J01NRETRYTEST", nullptr, time1Second, time10Minutes, nullptr, nullptr,
                                                nullptr, nullptr, joinerCallback, &fedIsJoined);
    nexus.AdvanceTime(time5Minutes);

    enableCommissionerOnLeader(nexus, leader);

    nexus.AdvanceTime(time30Seconds);
    VerifyOrQuit(fedIsJoined);
    fed.Get<Mle::Mle>().Start();
    nexus.AdvanceTime(time10Seconds);
    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());

    Log("Test passed.");
    Log("Network was simulated for a duration of %lu seconds\n\n\n\n",
        ToUlong(Time::MsecToSec(nexus.GetNow().GetValue())));
}

void TestJoinMultipleNodesAtOnce(void)
{
    Core nexus;
    Log("------------------------------- Test Joining Multiple Nodes At Once -------------------------------");
    Log("Start retrying joiner on multiple nodes at the same time as commissioner.");

    Node                    *leader;
    static constexpr uint8_t kNumJoiners              = 100;
    bool                     fedIsJoined[kNumJoiners] = {false};
    uint8_t                  joinerIndex              = 0;

    numCallbackExecuted = 0;

    for (uint8_t i = 0; i < kNumJoiners + 1; i++)
    {
        nexus.CreateNode();
    }

    setLogLevelAllNodes(nexus, kLogLevelWarn);
    nexus.AdvanceTime(0);

    leader = nexus.GetNodes().GetHead();
    leader->Form();
    nexus.AdvanceTime(time10Seconds + 3 * time1Second);
    VerifyOrQuit(leader->Get<Mle::Mle>().IsLeader());

    for (Node &node : nexus.GetNodes())
    {
        if (&node == leader)
        {
            continue;
        }
        prepareForJoining(node, Node::kAsFed);
    }

    nexus.AdvanceTime(time1Second);

    for (Node &node : nexus.GetNodes())
    {
        if (&node == leader)
        {
            continue;
        }

        VerifyOrQuit(!node.Get<MeshCoP::ActiveDatasetManager>().IsCommissioned());
        void *context = static_cast<void *>(&fedIsJoined[joinerIndex]);
        joinerIndex++;
        node.Get<MeshCoP::Joiner>().StartWithRetries("J01NRETRYTEST", nullptr, time1Second, time30Minutes, nullptr,
                                                     nullptr, nullptr, nullptr, joinerCallback, context);
    }

    nexus.AdvanceTime(time5Minutes);
    enableCommissionerOnLeader(nexus, *leader);
    nexus.AdvanceTime(time30Minutes);

    joinerIndex = 0;
    for (Node &node : nexus.GetNodes())
    {
        if (&node == leader)
        {
            continue;
        }
        // make sure all devices have joined, order does not matter
        VerifyOrQuit(fedIsJoined[joinerIndex]);
        joinerIndex++;
        node.Get<Mle::Mle>().Start();
    }
    Log("All nodes joined.");

    nexus.AdvanceTime(time10Seconds);

    for (Node &node : nexus.GetNodes())
    {
        if (&node == leader)
        {
            continue;
        }
        VerifyOrQuit(node.Get<Mle::Mle>().IsChild() || node.Get<Mle::Mle>().IsRouter());
    }
    Log("All nodes have valid role.");

    Log("Test passed.");
    Log("Network was simulated for a duration of %lu seconds.\n", ToUlong(Time::MsecToSec(nexus.GetNow().GetValue())));
}

void enableCommissionerOnLeader(Core &aCore, Node &aLeader)
{
    aLeader.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr);
    aCore.AdvanceTime(time10Seconds);
    VerifyOrQuit(aLeader.Get<MeshCoP::Commissioner>().IsActive());

    // timeout needs to be large enough to cover joiner retries
    aLeader.Get<MeshCoP::Commissioner>().AddJoinerAny("J01NRETRYTEST", 900);
    aCore.AdvanceTime(time10Seconds);
}

void disableCommissionerOnLeader(Core &aCore, Node &aLeader)
{
    aLeader.Get<MeshCoP::Commissioner>().RemoveJoinerAny(0);
    aCore.AdvanceTime(time10Seconds);
    aLeader.Get<MeshCoP::Commissioner>().Stop();
    aCore.AdvanceTime(time10Seconds);
    VerifyOrQuit(!aLeader.Get<MeshCoP::Commissioner>().IsActive());
}

void setLogLevelAllNodes(Nexus::Core &aCore, LogLevel aLogLevel)
{
    for (Node &node : aCore.GetNodes())
    {
        node.GetInstance().SetLogLevel(aLogLevel);
    }
}

void prepareForJoining(Node &aNode, Node::JoinMode aJoinMode)
{
    // Node::Join(...) is not used, as joining in tests is not done with leader dataset
    MeshCoP::Dataset dataset;
    Mle::DeviceMode  mode(0);

    switch (aJoinMode)
    {
    case Node::kAsFed:
        SuccessOrQuit(aNode.Get<Mle::Mle>().SetRouterEligible(false));
        OT_FALL_THROUGH;

    case Node::kAsFtd:
        mode.Set(Mle::DeviceMode::kModeRxOnWhenIdle | Mle::DeviceMode::kModeFullThreadDevice |
                 Mle::DeviceMode::kModeFullNetworkData);
        break;
    case Node::kAsMed:
        mode.Set(Mle::DeviceMode::kModeRxOnWhenIdle | Mle::DeviceMode::kModeFullNetworkData);
        break;
    case Node::kAsSed:
        mode.Set(Mle::DeviceMode::kModeFullNetworkData);
        break;
    }

    SuccessOrQuit(aNode.Get<Mle::Mle>().SetDeviceMode(mode));
    aNode.Get<ThreadNetif>().Up();
}

uint32_t numCallbackExecutedMin(uint32_t aTimeout)
{
    static constexpr uint32_t retryBaseTimeoutMax = 1255;
    static constexpr uint32_t joinAttemptTime     = 6000; // estimated maximal time for join attempt
    static constexpr uint32_t callbacksAtMaxDelay = 5;    // when backoff is at max (16), cb was called 5 times

    uint32_t remainingTime = 0;

    if (aTimeout <= retryBaseTimeoutMax * (1 + 2 + 4 + 8) + 4 * joinAttemptTime) // exponential backoff up to 16
    {
        return 1;
    }

    remainingTime = aTimeout - retryBaseTimeoutMax * (1 + 2 + 4 + 8) - callbacksAtMaxDelay * joinAttemptTime;
    return remainingTime / (retryBaseTimeoutMax * 16 + joinAttemptTime) + callbacksAtMaxDelay;
}

uint32_t numCallbackExecutedMax(uint32_t aTimeout)
{
    static constexpr uint32_t retryBaseTimeoutMin = 1000;
    static constexpr uint32_t joinAttemptTime     = 3000; // estimated minimal time for join attempt
    static constexpr uint32_t callbacksAtMaxDelay = 5;    // when backoff is at max (16), cb was called 5 times

    uint32_t remainingTime = 0;

    if (aTimeout <= retryBaseTimeoutMin * (1 + 2 + 4 + 8) + 4 * joinAttemptTime) // exponential backoff up to 16
    {
        return callbacksAtMaxDelay + 1;
    }

    remainingTime = aTimeout - retryBaseTimeoutMin * (1 + 2 + 4 + 8) - callbacksAtMaxDelay * joinAttemptTime;
    return remainingTime / (retryBaseTimeoutMin * 16 + joinAttemptTime) + callbacksAtMaxDelay + 1;
}

void joinerCallback(otError aResult, void *aContext)
{
    numCallbackExecuted++;
    if (aResult == OT_ERROR_NONE)
    {
        Log("Joiner joined successfully.");
        bool *joinSuccessful = static_cast<bool *>(aContext);
        *joinSuccessful      = true;
    }
    else
    {
        Log("Joiner failed to join. Error: %d: %s", aResult, otThreadErrorToString(aResult));
    }
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestJoiner();
    ot::Nexus::TestJoinerWithRetries();
    ot::Nexus::TestJoinerExecutesRetries();
    ot::Nexus::TestCommissionerStartsDuringRetryingJoiner();
    ot::Nexus::TestExecuteTwoJoiners();
    ot::Nexus::TestStoppingJoiner();
    ot::Nexus::TestJoinMultipleNodesAtOnce();
    printf("All tests passed.\n");
    return 0;
}
