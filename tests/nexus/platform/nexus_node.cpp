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

#include "nexus_node.hpp"
#include "nexus_utils.hpp"

namespace ot {
namespace Nexus {

void Node::Form(void)
{
    MeshCoP::Dataset::Info datasetInfo;

    SuccessOrQuit(datasetInfo.GenerateRandom(*this));
    Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);

    Get<ThreadNetif>().Up();
    SuccessOrQuit(Get<Mle::MleRouter>().Start());
}

void Node::Join(Node &aNode, JoinMode aJoinMode)
{
    MeshCoP::Dataset dataset;
    Mle::DeviceMode  mode(0);

    switch (aJoinMode)
    {
    case kAsFed:
        SuccessOrQuit(Get<Mle::MleRouter>().SetRouterEligible(false));
        OT_FALL_THROUGH;

    case kAsFtd:
        mode.Set(Mle::DeviceMode::kModeRxOnWhenIdle | Mle::DeviceMode::kModeFullThreadDevice |
                 Mle::DeviceMode::kModeFullNetworkData);
        break;
    case kAsMed:
        mode.Set(Mle::DeviceMode::kModeRxOnWhenIdle | Mle::DeviceMode::kModeFullNetworkData);
        break;
    case kAsSed:
        mode.Set(Mle::DeviceMode::kModeFullNetworkData);
        break;
    }

    SuccessOrQuit(Get<Mle::Mle>().SetDeviceMode(mode));

    SuccessOrQuit(aNode.Get<MeshCoP::ActiveDatasetManager>().Read(dataset));
    Get<MeshCoP::ActiveDatasetManager>().SaveLocal(dataset);

    Get<ThreadNetif>().Up();
    SuccessOrQuit(Get<Mle::MleRouter>().Start());
}

void Node::AllowList(Node &aNode)
{
    SuccessOrQuit(Get<Mac::Filter>().AddAddress(aNode.Get<Mac::Mac>().GetExtAddress()));
    Get<Mac::Filter>().SetMode(Mac::Filter::kModeAllowlist);
}

void Node::UnallowList(Node &aNode) { Get<Mac::Filter>().RemoveAddress(aNode.Get<Mac::Mac>().GetExtAddress()); }

} // namespace Nexus
} // namespace ot
