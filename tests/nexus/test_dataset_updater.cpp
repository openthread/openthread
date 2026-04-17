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

void VerifyChannel(Core &aNexus, uint16_t aChannel)
{
    for (Node &node : aNexus.GetNodes())
    {
        MeshCoP::Dataset::Info datasetInfo;
        SuccessOrQuit(node.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));

        VerifyOrQuit(datasetInfo.IsPresent<MeshCoP::Dataset::kChannel>());
        VerifyOrQuit(datasetInfo.Get<MeshCoP::Dataset::kChannel>() == aChannel);
        VerifyOrQuit(node.Get<Mac::Mac>().GetPanChannel() == aChannel);
    }
}

void TestDatasetUpdater(void)
{
    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();
    Node &med    = nexus.CreateNode();
    Node &sed    = nexus.CreateNode();

    leader.SetName("leader");
    router.SetName("router");
    med.SetName("med");
    sed.SetName("sed");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Form network on channel 11");

    leader.Form();
    {
        MeshCoP::Dataset::Info datasetInfo;
        SuccessOrQuit(leader.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(11);
        leader.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
    }

    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    med.Join(router, Node::kAsMed);
    sed.Join(router, Node::kAsSed);

    // Set a faster poll period for SED to ensure it receives Dataset updates promptly
    SuccessOrQuit(sed.Get<DataPollSender>().SetExternalPollPeriod(100));

    nexus.AdvanceTime(300 * 1000);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(med.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    VerifyChannel(nexus, 11);

    Log("---------------------------------------------------------------------------------------");
    Log("Update initiated by LEADER to channel 12");

    {
        MeshCoP::Dataset::Info datasetInfo;
        datasetInfo.Clear();
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(12);
        SuccessOrQuit(leader.Get<MeshCoP::DatasetUpdater>().RequestUpdate(datasetInfo, nullptr, nullptr));
    }

    nexus.AdvanceTime(200 * 1000);
    VerifyChannel(nexus, 12);

    Log("---------------------------------------------------------------------------------------");
    Log("Update initiated by ROUTER to channel 13");

    {
        MeshCoP::Dataset::Info datasetInfo;
        datasetInfo.Clear();
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(13);
        SuccessOrQuit(router.Get<MeshCoP::DatasetUpdater>().RequestUpdate(datasetInfo, nullptr, nullptr));
    }

    nexus.AdvanceTime(200 * 1000);
    VerifyChannel(nexus, 13);

    Log("---------------------------------------------------------------------------------------");
    Log("Update initiated by LEADER overridden by ROUTER (14 -> 15)");

    {
        MeshCoP::Dataset::Info datasetInfo;
        datasetInfo.Clear();
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(14);
        SuccessOrQuit(leader.Get<MeshCoP::DatasetUpdater>().RequestUpdate(datasetInfo, nullptr, nullptr));
    }
    nexus.AdvanceTime(20 * 1000);
    {
        MeshCoP::Dataset::Info datasetInfo;
        datasetInfo.Clear();
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(15);
        SuccessOrQuit(router.Get<MeshCoP::DatasetUpdater>().RequestUpdate(datasetInfo, nullptr, nullptr));
    }

    nexus.AdvanceTime(200 * 1000);
    VerifyChannel(nexus, 15);

    Log("---------------------------------------------------------------------------------------");
    Log("Update initiated by ROUTER overridden by LEADER (16 -> 17)");

    {
        MeshCoP::Dataset::Info datasetInfo;
        datasetInfo.Clear();
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(16);
        SuccessOrQuit(router.Get<MeshCoP::DatasetUpdater>().RequestUpdate(datasetInfo, nullptr, nullptr));
    }
    nexus.AdvanceTime(10 * 1000);
    {
        MeshCoP::Dataset::Info datasetInfo;
        datasetInfo.Clear();
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(17);
        SuccessOrQuit(leader.Get<MeshCoP::DatasetUpdater>().RequestUpdate(datasetInfo, nullptr, nullptr));
    }

    nexus.AdvanceTime(200 * 1000);
    VerifyChannel(nexus, 17);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestDatasetUpdater();
    printf("All tests passed\n");
    return 0;
}
