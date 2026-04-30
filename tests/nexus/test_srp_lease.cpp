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

#include <stdio.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static const uint16_t kNumNodes[] = {1, 5, 10}; // Number of nodes to use in different test runs

constexpr uint16_t kTestIterationsPerNumNodes = 1; // Number of iterations to run per num-node

enum State : uint8_t
{
    kNotRegistered, // No SRP registration yet
    kRegistered,    // SRP service registered
    kUnregistered,  // Unregistered the previously registered service (retaining name)
    kRemoved,       // Fully removed host and service (not retaining key-lease)
    kDisconnected,  // Disconnected (checking if the server correctly expires the previously registered service)
    kValidated,     // Final state of all nodes indicating validated server behavior
};

constexpr uint16_t kMaxNodes = 40;

// Test advances time in `kStepInterval`
constexpr uint32_t kStepInterval = 120 * TimeMilli::kOneSecondInMsec;
constexpr uint16_t kStepJitter   = 10 * TimeMilli::kOneSecondInMsec;

constexpr uint32_t kLeaseInterval = 7200 * TimeMilli::kOneSecondInMsec;

// All probabilities are percentage times 10
constexpr uint16_t kRegisterProbability   = 8;
constexpr uint16_t kUpdateRegProbability  = 8;
constexpr uint16_t kUnregisterProbability = 1;
constexpr uint16_t kRemoveProbability     = 1;
constexpr uint16_t kDisconnectProbability = 4;

static const char *StateToString(State aState)
{
#define StateMapList(_)                \
    _(kNotRegistered, "NotRegistered") \
    _(kRegistered, "Registered")       \
    _(kUnregistered, "Unregistered")   \
    _(kRemoved, "Removed")             \
    _(kDisconnected, "Disconnected")   \
    _(kValidated, "Validated")

    DefineEnumStringArray(StateMapList);

    return kStrings[aState];
}

struct NodeInfo
{
    void SetState(State aState, uint16_t aNodeId)
    {
        Log("Node %u: %s", aNodeId, StateToString(aState));

        mState      = aState;
        mUpdateTime = TimerMilli::GetNow();
    }

    State                mState;
    TimeMilli            mUpdateTime;
    Srp::Client::Service mService;
    const char          *mSubTypeLabels[5];
};

NodeInfo sInfo[kMaxNodes];

static void RegisterSrp(Node &aNode)
{
    NodeInfo             *info;
    Srp::Client::Service *service;

    VerifyOrQuit(aNode.GetId() < kMaxNodes);

    info    = &sInfo[aNode.GetId()];
    service = &info->mService;

    aNode.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);

    SuccessOrQuit(aNode.Get<Srp::Client>().EnableAutoHostAddress());
    SuccessOrQuit(aNode.Get<Srp::Client>().SetHostName(aNode.GetName()));

    ClearAllBytes(*service);
    service->mName          = "_test._udp";
    service->mInstanceName  = aNode.GetName();
    service->mSubTypeLabels = nullptr;
    service->mTxtEntries    = nullptr;
    service->mNumTxtEntries = 0;
    service->mPort          = 1000 + aNode.GetId();

    SuccessOrQuit(aNode.Get<Srp::Client>().AddService(*service));
}

static void UpdateSrpRegistration(Node &aNode)
{
    NodeInfo             *info;
    Srp::Client::Service *service;

    VerifyOrQuit(aNode.GetId() < kMaxNodes);

    aNode.Get<Srp::Client>().ClearHostAndServices();

    info    = &sInfo[aNode.GetId()];
    service = &info->mService;

    SuccessOrQuit(aNode.Get<Srp::Client>().EnableAutoHostAddress());
    SuccessOrQuit(aNode.Get<Srp::Client>().SetHostName(aNode.GetName()));

    ClearAllBytes(*service);
    service->mName          = "_test._udp";
    service->mInstanceName  = aNode.GetName();
    service->mSubTypeLabels = nullptr;
    service->mTxtEntries    = nullptr;
    service->mNumTxtEntries = 0;
    service->mPort          = Random::NonCrypto::GetUint16InRange(0x100, 0xff00);
    service->mSubTypeLabels = &info->mSubTypeLabels[Random::NonCrypto::GetUint8InRange(0, 4)];

    SuccessOrQuit(aNode.Get<Srp::Client>().AddService(*service));
}

static bool ShouldPerform(uint16_t aProbability)
{
    // Uses the given probability to randomly decide whether a certain action should be performed.

    return Random::NonCrypto::GetUint16InRange(0, 1000) < aProbability;
}

static const Srp::Server::Host *FindHost(Node &aServer, const char *aName)
{
    // Find a host on `aServer` SRP server matching `aName`

    static constexpr uint16_t kNameStringSize = 400;

    const Srp::Server::Host *host = nullptr;
    String<kNameStringSize>  fullName;

    fullName.Append("%s.default.service.arpa.", aName);

    while (true)
    {
        host = aServer.Get<Srp::Server>().GetNextHost(host);

        if (host == nullptr)
        {
            break;
        }

        if (StringMatch(host->GetFullName(), fullName.AsCString()))
        {
            break;
        }
    }

    return host;
}

void LogSrpServer(Node &aServer)
{
    const Srp::Server::Host *host = nullptr;

    Log("- - - - - -");

    while (true)
    {
        host = aServer.Get<Srp::Server>().GetNextHost(host);

        if (host == nullptr)
        {
            break;
        }

        Log("%s, is-deleted:%s", host->GetFullName(), ToYesNo(host->IsDeleted()));
    }
}

void TestSrpLease(uint16_t aNumNodes)
{
    Core  nexus;
    Node *nodes[kMaxNodes];
    Node *leader;

    VerifyOrQuit(aNumNodes + 1 <= kMaxNodes);

    Log("----------------------------------------------------------------------------------------------------------");
    Log("Testing with %u nodes", aNumNodes);

    leader = &nexus.CreateNode();

    leader->Form();
    nexus.AdvanceTime(100 * Time::kOneSecondInMsec);
    VerifyOrQuit(leader->Get<Mle::Mle>().IsLeader());

    leader->Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(30 * Time::kOneSecondInMsec);

    VerifyOrQuit(leader->Get<Srp::Server>().GetState() == Srp::Server::kStateRunning);

    nodes[0] = leader;

    for (uint16_t i = 1; i <= aNumNodes; i++)
    {
        sInfo[i].mState = kNotRegistered;

        sInfo[i].mSubTypeLabels[0] = "_s1";
        sInfo[i].mSubTypeLabels[1] = "_xvab";
        sInfo[i].mSubTypeLabels[2] = "_I12345678";
        sInfo[i].mSubTypeLabels[3] = "_cm";
        sInfo[i].mSubTypeLabels[4] = nullptr;

        nodes[i] = &nexus.CreateNode();

        nodes[i]->SetName("client", i);
        nodes[i]->Join(*leader, Node::kAsFed);
        nexus.AdvanceTime(100);
    }

    nexus.AdvanceTime(10 * Time::kOneSecondInMsec);

    for (uint16_t i = 1; i <= aNumNodes; i++)
    {
        VerifyOrQuit(nodes[i]->Get<Mle::Mle>().IsChild());
    }

    while (true)
    {
        uint16_t nodesValidated = 0;

        nexus.AdvanceTime(Random::NonCrypto::AddJitter(kStepInterval, kStepJitter));

        // LogSrpServer(*leader);

        for (uint16_t i = 1; i <= aNumNodes; i++)
        {
            NodeInfo                &info = sInfo[i];
            const Srp::Server::Host *host;

            switch (info.mState)
            {
            case kNotRegistered:
                if (ShouldPerform(kRegisterProbability))
                {
                    RegisterSrp(*nodes[i]);
                    nexus.AdvanceTime(1000);
                    VerifyOrQuit(nodes[i]->Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);
                    info.SetState(kRegistered, i);
                }

                break;

            case kRegistered:
                host = FindHost(*leader, nodes[i]->GetName());
                VerifyOrQuit(host != nullptr);
                VerifyOrQuit(!host->IsDeleted());
                VerifyOrQuit(!host->GetServices().IsEmpty());
                VerifyOrQuit(!host->GetServices().GetHead()->IsDeleted());

                if (ShouldPerform(kUpdateRegProbability))
                {
                    UpdateSrpRegistration(*nodes[i]);
                    nexus.AdvanceTime(1000);
                    VerifyOrQuit(nodes[i]->Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);
                    Log("Node %u: Updated SRP", i);
                }
                else if (ShouldPerform(kDisconnectProbability))
                {
                    nodes[i]->Reset();
                    info.SetState(kDisconnected, i);
                }
                else if (ShouldPerform(kUnregisterProbability))
                {
                    SuccessOrQuit(nodes[i]->Get<Srp::Client>().RemoveHostAndServices(/* aRemoveKeyLease */ false));
                    info.SetState(kUnregistered, i);
                }
                else if (ShouldPerform(kRemoveProbability))
                {
                    SuccessOrQuit(nodes[i]->Get<Srp::Client>().RemoveHostAndServices(/* aRemoveKeyLease */ true));
                    info.SetState(kRemoved, i);
                }
                break;

            case kUnregistered:
                // If the client unregistered (retaining the key-lease), the `host`
                // on the server must be marked as deleted.

                host = FindHost(*leader, nodes[i]->GetName());
                VerifyOrQuit(host != nullptr);
                VerifyOrQuit(host->IsDeleted());
                if (!host->GetServices().IsEmpty())
                {
                    VerifyOrQuit(host->GetServices().GetHead()->IsDeleted());
                }

                info.SetState(kValidated, i);
                break;

            case kRemoved:
                // If the client fully removes the registration, the `host` on
                // the server must be fully removed.

                host = FindHost(*leader, nodes[i]->GetName());
                VerifyOrQuit(host == nullptr);
                info.SetState(kValidated, i);
                break;

            case kDisconnected:
                // If the client disconnects without changing its registration,
                // we ensure the `host` on the server is marked as deleted once
                // its lease time expires.

                host = FindHost(*leader, nodes[i]->GetName());

                if (host != nullptr)
                {
                    if (host->IsDeleted())
                    {
                        VerifyOrQuit(!host->GetServices().IsEmpty());
                        VerifyOrQuit(host->GetServices().GetHead()->IsDeleted());
                        info.SetState(kValidated, i);
                    }
                    else
                    {
                        if (info.mUpdateTime + kLeaseInterval < TimerMilli::GetNow())
                        {
                            Log("Node %u is still on server beyond its lease time", i);
                            VerifyOrQuit(false);
                        }
                    }
                }
                else
                {
                    info.SetState(kValidated, i);
                }

                break;

            case kValidated:
                nodesValidated++;
            }
        }

        if (nodesValidated == aNumNodes)
        {
            Log("Test passed successfully");
            break;
        }
    }
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    for (uint16_t numNodes : ot::Nexus::kNumNodes)
    {
        for (uint16_t iter = 0; iter < ot::Nexus::kTestIterationsPerNumNodes; iter++)
        {
            ot::Nexus::TestSrpLease(numNodes);
        }
    }

    printf("All tests passed\n");
    return 0;
}
