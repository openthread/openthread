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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static constexpr uint16_t kNumberOfRoles = (Mle::kRoleLeader + 1);

typedef uint16_t RoleStats[kNumberOfRoles];

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

void Test(void)
{
    static constexpr uint16_t kNumNodes = 200;

    // All times in msec
    static constexpr uint32_t kMaxWaitTime            = 20 * Time::kOneMinuteInMsec;
    static constexpr uint32_t kStatCollectionInterval = 125;
    static constexpr uint32_t kExtraSimulTimAfterPass = 5 * Time::kOneSecondInMsec;

    Core      nexus;
    Node     *leader;
    RoleStats roleStats;

    for (uint16_t i = 0; i < kNumNodes; i++)
    {
        nexus.CreateNode();
    }

    nexus.AdvanceTime(0);

    Log("Starting %u nodes simultaneously", kNumNodes);

    leader = nexus.GetNodes().GetHead();
    leader->Form();

    for (Node &node : nexus.GetNodes())
    {
        if (&node == leader)
        {
            continue;
        }

        node.Join(*leader);
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

    Log("=========================================================");
    Log("All nodes are now part of the same partition");
    Log("Network stabilized after %u sec", nexus.GetNow().GetValue() / Time::kOneSecondInMsec);
    Log("Continue simulation for another %u sec", kExtraSimulTimAfterPass / Time::kOneSecondInMsec);
    Log("=========================================================");

    for (uint32_t step = 0; step < kExtraSimulTimAfterPass / kStatCollectionInterval; step++)
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
