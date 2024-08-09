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

void TestFormJoin(void)
{
    // Validate basic operations, forming a network and
    // joining as router, FED, MED, and SED

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &fed     = nexus.CreateNode();
    Node &sed     = nexus.CreateNode();
    Node &med     = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();

    nexus.AdvanceTime(0);

    for (Node &node : nexus.GetNodes())
    {
        node.GetInstance().SetLogLevel(kLogLevelInfo);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Form network");

    leader.Form();
    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Join an FED");

    fed.Join(leader, Node::kAsFed);
    nexus.AdvanceTime(2 * 1000);
    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Join an SED");

    sed.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(2 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Join an MED");

    med.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(2 * 1000);
    VerifyOrQuit(med.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Join two routers");

    router1.Join(leader);
    router2.Join(leader);

    Log("---------------------------------------------------------------------------------------");
    Log("Check all nodes roles and device modes");

    nexus.AdvanceTime(300 * 1000);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    VerifyOrQuit(fed.Get<Mle::Mle>().IsRxOnWhenIdle());
    VerifyOrQuit(fed.Get<Mle::Mle>().IsFullThreadDevice());

    VerifyOrQuit(med.Get<Mle::Mle>().IsRxOnWhenIdle());
    VerifyOrQuit(!med.Get<Mle::Mle>().IsFullThreadDevice());
    VerifyOrQuit(med.Get<Mle::Mle>().IsMinimalEndDevice());

    VerifyOrQuit(!sed.Get<Mle::Mle>().IsRxOnWhenIdle());
    VerifyOrQuit(!sed.Get<Mle::Mle>().IsFullThreadDevice());
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestFormJoin();
    printf("All tests passed\n");
    return 0;
}
