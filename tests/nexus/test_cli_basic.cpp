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

void TestCliBasic(void)
{
    // Validate basic CLI commands.

    static constexpr uint16_t kNumRouters = 8;

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node *routers[kNumRouters];

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNone));

    Log("---------------------------------------------------------------------------------------");
    Log("Initial state");

    VerifyOrQuit(leader.GetCliOutputLines().GetLength() == 0);

    leader.InputCli("state");
    VerifyOrQuit(leader.IsCliOutputSuccess());
    VerifyOrQuit(leader.GetCliOutputLines()[0].Matches("disabled"));

    Log("---------------------------------------------------------------------------------------");
    Log("Form network on `leader`");

    leader.InputCli("dataset init new");
    VerifyOrQuit(leader.IsCliOutputSuccess());

    leader.InputCli("dataset");

    Log("`dataset` command output on `leader`");

    for (const Node::CliOutputLine &line : leader.GetCliOutputLines())
    {
        Log("- %s", line.GetLine());
    }

    leader.InputCli("dataset commit active");
    VerifyOrQuit(leader.IsCliOutputSuccess());

    leader.InputCli("ifconfig up");
    VerifyOrQuit(leader.IsCliOutputSuccess());

    leader.InputCli("thread start");
    VerifyOrQuit(leader.IsCliOutputSuccess());

    nexus.AdvanceTime(2 * Time::kOneMinuteInMsec);

    leader.InputCli("state");
    VerifyOrQuit(leader.IsCliOutputSuccess());
    VerifyOrQuit(leader.GetCliOutputLines()[0].Matches("leader"));

    Log("---------------------------------------------------------------------------------------");
    Log("Join %u routers to same network", kNumRouters);

    for (Node *&router : routers)
    {
        router = &nexus.CreateNode();

        router->InputCli("state");
        VerifyOrQuit(router->IsCliOutputSuccess());
        VerifyOrQuit(router->GetCliOutputLines()[0].Matches("disabled"));

        router->InputCli("dataset clear");

        // Set network key and channel

        leader.InputCli("networkkey");
        VerifyOrQuit(leader.IsCliOutputSuccess());
        router->InputCli("dataset networkkey %s", leader.GetCliOutputLines()[0].GetLine());
        VerifyOrQuit(router->IsCliOutputSuccess());

        leader.InputCli("channel");
        VerifyOrQuit(leader.IsCliOutputSuccess());
        router->InputCli("dataset channel %s", leader.GetCliOutputLines()[0].GetLine());
        VerifyOrQuit(router->IsCliOutputSuccess());

        router->InputCli("dataset commit active");
        VerifyOrQuit(router->IsCliOutputSuccess());

        router->InputCli("ifconfig up");
        VerifyOrQuit(router->IsCliOutputSuccess());

        router->InputCli("thread start");
        VerifyOrQuit(router->IsCliOutputSuccess());

        nexus.AdvanceTime(100 * Time::kOneSecondInMsec);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Make sure all routers are attached");

    nexus.AdvanceTime(7 * Time::kOneMinuteInMsec);

    for (Node *router : routers)
    {
        router->InputCli("state");
        VerifyOrQuit(router->IsCliOutputSuccess());
        VerifyOrQuit(router->GetCliOutputLines()[0].Matches("router"));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("The neighbor table");

    leader.InputCli("neighbor table");
    VerifyOrQuit(leader.IsCliOutputSuccess());

    for (const Node::CliOutputLine &line : leader.GetCliOutputLines())
    {
        Log("- %s", line.GetLine());
    }

    Log("---------------------------------------------------------------------------------------");
    Log("The router table");

    leader.InputCli("router table");
    VerifyOrQuit(leader.IsCliOutputSuccess());

    for (const Node::CliOutputLine &line : leader.GetCliOutputLines())
    {
        Log("- %s", line.GetLine());
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Check behavior with an invalid CLI command");

    leader.InputCli("invalidcommand");
    VerifyOrQuit(!leader.IsCliOutputSuccess());
    VerifyOrQuit(leader.GetCliOutputLines().GetLength() == 1);
    VerifyOrQuit(leader.GetCliOutputLines()[0].StartsWith("Error "));

    for (const Node::CliOutputLine &line : leader.GetCliOutputLines())
    {
        Log("- %s", line.GetLine());
    }
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestCliBasic();
    printf("All tests passed\n");
    return 0;
}
