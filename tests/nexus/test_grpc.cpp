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

#include "openthread-core-config.h"

#include <grpcpp/grpcpp.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_grpc.hpp"
#include "platform/nexus_node.hpp"
#include "platform/nexus_sim.hpp"

#include "simulation.grpc.pb.h"

namespace ot {
namespace Nexus {

void TestGrpc(void)
{
    Core       nexus;
    Simulation sim;
    GrpcServer server;

    auto channel = grpc::CreateChannel(kDefaultGrpcServerAddress, grpc::InsecureChannelCredentials());

    // Wait for the channel to be ready with a timeout
    VerifyOrQuit(channel->WaitForConnected(std::chrono::system_clock::now() + std::chrono::seconds(5)),
                 "gRPC channel failed to connect within timeout");

    auto stub = nexus::NexusService::NewStub(channel);

    uint32_t leaderId;

    Log("---------------------------------------------------------------------------------------");
    Log("Test CreateNode RPC");
    {
        nexus::CreateNodeRequest request;
        request.set_x(10.0);
        request.set_y(20.0);
        nexus::CreateNodeResponse response;
        grpc::ClientContext       context;
        grpc::Status              status = stub->CreateNode(&context, request, &response);

        VerifyOrQuit(status.ok(), "CreateNode RPC failed");
        leaderId = response.node_id();
        VerifyOrQuit(leaderId == 0, "Invalid node ID returned");

        std::lock_guard<Simulation> lock(sim);
        Node                       *node = nexus.FindNodeById(leaderId);
        VerifyOrQuit(node != nullptr, "Node not found in Core");
        VerifyOrQuit(node->GetPositionX() == 10.0, "Wrong X position");
        VerifyOrQuit(node->GetPositionY() == 20.0, "Wrong Y position");
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Test SetSpeed RPC");
    {
        nexus::SetSpeedRequest request;
        request.set_speed_factor(2.5);
        nexus::SetSpeedResponse response;
        grpc::ClientContext     context;
        grpc::Status            status = stub->SetSpeed(&context, request, &response);

        VerifyOrQuit(status.ok(), "SetSpeed RPC failed");

        {
            std::lock_guard<Simulation> lock(sim);
            VerifyOrQuit(sim.GetSpeedFactor() == 2.5, "Speed factor not set correctly");
        }
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Test StreamEvents");
    {
        grpc::ClientContext  context;
        nexus::StreamRequest request;
        auto                 reader = stub->StreamEvents(&context, request);

        // The StreamEvents call should trigger DumpState
        nexus::SimulationEvent event;
        VerifyOrQuit(reader->Read(&event), "Failed to read event from stream");

        // Push an event manually to verify it comes through
        nexus::SimulationEvent manualEvent;
        manualEvent.set_timestamp_us(987654);
        server.PushEvent(manualEvent);

        bool foundManualEvent = false;
        // We might get multiple events (DumpState pushes many)
        for (int i = 0; i < 100; i++)
        {
            if (!reader->Read(&event))
                break;
            if (event.timestamp_us() == 987654)
            {
                foundManualEvent = true;
                break;
            }
        }
        VerifyOrQuit(foundManualEvent, "Failed to receive manual event via stream");
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Test SetNodePosition RPC");
    {
        nexus::SetNodePositionRequest request;
        request.set_node_id(leaderId);
        request.set_x(30.0);
        request.set_y(40.0);
        nexus::SetNodePositionResponse response;
        grpc::ClientContext            context;
        grpc::Status                   status = stub->SetNodePosition(&context, request, &response);

        VerifyOrQuit(status.ok(), "SetNodePosition RPC failed");

        std::lock_guard<Simulation> lock(sim);
        Node                       *node = nexus.FindNodeById(leaderId);
        VerifyOrQuit(node != nullptr, "Leader node not found");
        VerifyOrQuit(node->GetPositionX() == 30.0, "Wrong X position after SetNodePosition");
        VerifyOrQuit(node->GetPositionY() == 40.0, "Wrong Y position after SetNodePosition");
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Test FormNetwork RPC");
    {
        nexus::FormNetworkRequest request;
        request.set_node_id(leaderId);
        nexus::FormNetworkResponse response;
        grpc::ClientContext        context;
        grpc::Status               status = stub->FormNetwork(&context, request, &response);

        VerifyOrQuit(status.ok(), "FormNetwork RPC failed");

        // Advancing time to allow forming
        {
            std::lock_guard<Simulation> lock(sim);
            nexus.AdvanceTime(13000);
        }

        std::lock_guard<Simulation> lock(sim);
        Node                       *node = nexus.FindNodeById(leaderId);
        VerifyOrQuit(node != nullptr, "Leader node not found");
        VerifyOrQuit(node->Get<Mle::Mle>().IsLeader(), "Node failed to become leader");
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Test JoinNetwork RPC");
    {
        nexus::CreateNodeRequest createRequest;
        createRequest.set_x(50.0);
        createRequest.set_y(50.0);
        nexus::CreateNodeResponse createResponse;
        grpc::ClientContext       createCtx;
        stub->CreateNode(&createCtx, createRequest, &createResponse);
        uint32_t joinerId = createResponse.node_id();

        nexus::JoinNetworkRequest joinRequest;
        joinRequest.set_node_id(joinerId);
        joinRequest.set_target_node_id(leaderId);
        joinRequest.set_mode(nexus::JOIN_MODE_FTD);
        nexus::JoinNetworkResponse joinResponse;
        grpc::ClientContext        joinCtx;
        grpc::Status               status = stub->JoinNetwork(&joinCtx, joinRequest, &joinResponse);

        VerifyOrQuit(status.ok(), "JoinNetwork RPC failed");

        {
            std::lock_guard<Simulation> lock(sim);
            nexus.AdvanceTime(10000);
        }

        std::lock_guard<Simulation> lock(sim);
        Node                       *joiner = nexus.FindNodeById(joinerId);
        VerifyOrQuit(joiner != nullptr, "Joiner node not found");
        VerifyOrQuit(joiner->Get<Mle::Mle>().IsChild() || joiner->Get<Mle::Mle>().IsRouter(),
                     "Node failed to join network");
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Test SetNodeState RPC (Disable)");
    {
        nexus::SetNodeStateRequest request;
        request.set_node_id(leaderId);
        request.set_enabled(false);
        nexus::SetNodeStateResponse response;
        grpc::ClientContext         context;
        grpc::Status                status = stub->SetNodeState(&context, request, &response);

        VerifyOrQuit(status.ok(), "SetNodeState (Disable) failed");

        std::lock_guard<Simulation> lock(sim);
        Node                       *node = nexus.FindNodeById(leaderId);
        VerifyOrQuit(node != nullptr, "Leader node not found");
        VerifyOrQuit(node->Get<Mle::Mle>().GetRole() == ot::Mle::kRoleDisabled, "Node role not disabled");
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Test SetNodeState RPC (Enable)");
    {
        nexus::SetNodeStateRequest request;
        request.set_node_id(leaderId);
        request.set_enabled(true);
        nexus::SetNodeStateResponse response;
        grpc::ClientContext         context;
        grpc::Status                status = stub->SetNodeState(&context, request, &response);

        VerifyOrQuit(status.ok(), "SetNodeState (Enable) failed");

        {
            std::lock_guard<Simulation> lock(sim);
            nexus.AdvanceTime(1000);
        }

        std::lock_guard<Simulation> lock(sim);
        Node                       *node = nexus.FindNodeById(leaderId);
        VerifyOrQuit(node != nullptr, "Leader node not found");
        VerifyOrQuit(node->Get<Mle::Mle>().GetRole() != ot::Mle::kRoleDisabled, "Node role still disabled");
    }
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestGrpc();
    ot::Nexus::Log("All tests passed");
    return 0;
}
