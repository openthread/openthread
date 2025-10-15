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

static constexpr uint16_t kNumberOfRoles = (Mle::kRoleLeader + 1);

typedef uint16_t RoleStats[kNumberOfRoles];

struct SrpStatus
{
    uint16_t mNumRunning;
    uint16_t mNumRegistered;
};

static void CaculateRoleStats(Core &aNexus, RoleStats &aRoleStats)
{
    ClearAllBytes(aRoleStats);

    for (Node &node : aNexus.GetNodes())
    {
        aRoleStats[node.Get<Mle::Mle>().GetRole()]++;
    }
}

static bool CheckRoleStats(const RoleStats &aRoleStats)
{
    return (aRoleStats[Mle::kRoleLeader] == 1) && (aRoleStats[Mle::kRoleDetached] == 0) &&
           (aRoleStats[Mle::kRoleDisabled] == 0);
}

static void CalculateSrpStatus(Core &aNexus, SrpStatus &aStatus)
{
    ClearAllBytes(aStatus);

    for (Node &node : aNexus.GetNodes())
    {
        if (!node.Get<Srp::Client>().IsAutoStartModeEnabled())
        {
            continue;
        }

        if (node.Get<Srp::Client>().IsRunning())
        {
            aStatus.mNumRunning++;
        }

        switch (node.Get<Srp::Client>().GetHostInfo().GetState())
        {
        case Srp::Client::kToRefresh:
        case Srp::Client::kRefreshing:
        case Srp::Client::kRegistered:
            aStatus.mNumRegistered++;
            break;
        default:
            break;
        }
    }

    Log("| %15u | %15u | ", aStatus.mNumRunning, aStatus.mNumRegistered);
}

void Test(void)
{
    static constexpr uint16_t kNumNodes = 100;

    // All times in msec
    static constexpr uint32_t kMaxWaitTime            = 20 * Time::kOneMinuteInMsec;
    static constexpr uint32_t kStatCollectionInterval = 200;
    static constexpr uint32_t kExtraSimulTimAfterPass = 5 * Time::kOneSecondInMsec;
    static constexpr uint32_t kMaxSrpWaitTime         = 20 * Time::kOneSecondInMsec;

    Core      nexus;
    Node     *leader;
    RoleStats roleStats;

    Srp::Client::Service services[kNumNodes];
    String<100>          hostNames[kNumNodes];
    String<100>          serviceNames[kNumNodes];
    SrpStatus            srpStatus;
    uint64_t             srpStartTime;
    uint64_t             allRegDuration;

    for (uint16_t i = 0; i < kNumNodes; i++)
    {
        nexus.CreateNode();
    }

    nexus.AdvanceTime(0);

    Log("Starting %u nodes", kNumNodes);

    leader = nexus.GetNodes().GetHead();
    leader->Form();

    for (Node &node : nexus.GetNodes())
    {
        // node.GetInstance().SetLogLevel(kLogLevelInfo);

        if (&node == leader)
        {
            continue;
        }

        node.Join(*leader);
        nexus.AdvanceTime(500);
    }

    for (uint32_t step = 0; step < kMaxWaitTime / kStatCollectionInterval; step++)
    {
        if ((step % 20) == 0)
        {
            Log("+----------+----------+----------+----------+----------+");
            Log("| Leader   | Router   | Child    | Detached | Disabled |");
            Log("+----------+----------+----------+----------+----------+");
        }

        CaculateRoleStats(nexus, roleStats);

        Log("| %8u | %8u | %8u | %8u | %8u |", roleStats[Mle::kRoleLeader], roleStats[Mle::kRoleRouter],
            roleStats[Mle::kRoleChild], roleStats[Mle::kRoleDetached], roleStats[Mle::kRoleDisabled]);

        nexus.AdvanceTime(kStatCollectionInterval);

        if (CheckRoleStats(roleStats))
        {
            break;
        }
    }

    VerifyOrQuit(CheckRoleStats(roleStats));

    Log("Register an SRP service on all nodes");

    for (Node &node : nexus.GetNodes())
    {
        uint32_t index = node.GetId();

        if (&node == leader)
        {
            continue;
        }

        hostNames[index].Append("host%u", index);

        node.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
        SuccessOrQuit(node.Get<Srp::Client>().EnableAutoHostAddress());

        SuccessOrQuit(node.Get<Srp::Client>().SetHostName(hostNames[index].AsCString()));

        ClearAllBytes(services[index]);
        serviceNames[index].Append("svr%u", index);
        services[index].mName         = "_test._udp";
        services[index].mInstanceName = serviceNames[index].AsCString();
        services[index].mPort         = 5000 + index;

        SuccessOrQuit(node.Get<Srp::Client>().AddService(services[index]));
    }

    Log("+-----------------+-----------------+");
    Log("| Running         | Registered      |");
    Log("+-----------------+-----------------+");

    CalculateSrpStatus(nexus, srpStatus);

    nexus.AdvanceTime(20000);

    Log("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    Log("Starting SRP server on leader");
    Log("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");

    SuccessOrQuit(leader->Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    leader->Get<Srp::Server>().SetEnabled(true);

    srpStartTime = leader->Get<Uptime>().GetUptime();

    for (uint32_t step = 0; step < kMaxSrpWaitTime / kStatCollectionInterval; step++)
    {
        if ((step % 20) == 0)
        {
            Log("+-----------------+-----------------+");
            Log("| Running         | Registered      |");
            Log("+-----------------+-----------------+");
        }

        CalculateSrpStatus(nexus, srpStatus);

        nexus.AdvanceTime(kStatCollectionInterval);

        if (srpStatus.mNumRegistered == kNumNodes - 1)
        {
            break;
        }
    }

    allRegDuration = leader->Get<Uptime>().GetUptime() - srpStartTime;
    Log("All devices registered in %u.%03u sec", allRegDuration / 1000, allRegDuration % 1000);

    for (Node &node : nexus.GetNodes())
    {
        if (!node.Get<Srp::Client>().IsRunning())
        {
            continue;
        }

        Log("- Node %3u: Reason:%s, jitterMax:%u, actual-delay:%u", node.GetId(),
            Srp::Client::TxJitter::ReasonToString(node.Get<Srp::Client>().mTxJitter.mLastReason),
            node.Get<Srp::Client>().mTxJitter.mLastMaxJitter, node.Get<Srp::Client>().mTxJitter.mLastDelay);
    }
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test();
    printf("All tests passed\n");
    return 0;
}
