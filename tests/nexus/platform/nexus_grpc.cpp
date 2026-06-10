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

#include "nexus_grpc.hpp"
#include "nexus_sim.hpp"

#if OPENTHREAD_NEXUS_CONFIG_GRPC_ENABLE

#include <cerrno>
#include <chrono>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <grpcpp/grpcpp.h>
#include <unistd.h>

#include "nexus_logging.hpp"
#include "nexus_node.hpp"
#include "nexus_radio.hpp"
#include "nexus_radio_model.hpp"
#include "simulation.grpc.pb.h"
#include "common/clearable.hpp"
#include "common/code_utils.hpp"
#include "mac/mac_frame.hpp"
#include "thread/neighbor_table.hpp"

using namespace std::chrono_literals;

namespace ot {
namespace Nexus {

void GrpcServer::InitEvent(nexus::SimulationEvent &aEvent) { aEvent.set_timestamp_us(Core::Get().GetNowMicro64()); }

nexus::NodeRole GrpcServer::GetEnhancedRole(Node *aNode)
{
    const Mle::Mle &mle  = aNode->Get<Mle::Mle>();
    Mle::DeviceRole role = mle.GetRole();

    switch (role)
    {
    case Mle::kRoleDisabled:
        return nexus::ROLE_DISABLED;
    case Mle::kRoleDetached:
        return nexus::ROLE_DETACHED;
    case Mle::kRoleLeader:
        return nexus::ROLE_LEADER;
    case Mle::kRoleRouter:
        return nexus::ROLE_ROUTER;
    case Mle::kRoleChild:
        if (mle.IsFullThreadDevice())
        {
            return mle.IsRouterRoleAllowed() ? nexus::ROLE_REED : nexus::ROLE_FED;
        }

        return mle.IsRxOnWhenIdle() ? nexus::ROLE_MED : nexus::ROLE_SED;

    default:
        break;
    }

    return nexus::ROLE_UNKNOWN;
}

class GrpcServer::ServiceImpl final : public nexus::NexusService::Service
{
public:
    ServiceImpl(GrpcServer *aServer)
        : mServerPtr(aServer)
    {
    }

    grpc::Status StreamEvents(grpc::ServerContext *aContext,
                              const nexus::StreamRequest * /* aRequest */,
                              grpc::ServerWriter<nexus::SimulationEvent> *aWriter) override
    {
        static constexpr uint32_t kPopEventTimeoutMs = 1000;
        grpc::Status              status             = grpc::Status::OK;
        auto                     *queue              = mServerPtr->AddEventQueue();

        mServerPtr->ClientConnected();
        aWriter->SendInitialMetadata();
        mServerPtr->DumpState(queue);

        while (!aContext->IsCancelled())
        {
            nexus::SimulationEvent event;

            if (mServerPtr->PopEvent(queue, event, kPopEventTimeoutMs))
            {
                if (!aWriter->Write(event))
                {
                    break;
                }
            }
            else
            {
                if (!mServerPtr->IsRunning())
                {
                    break;
                }

                {
                    std::lock_guard<Simulation> lock(Simulation::Get());
                    mServerPtr->InitEvent(event);
                }
                event.mutable_heartbeat();

                if (!aWriter->Write(event))
                {
                    break;
                }
            }
        }

        mServerPtr->RemoveEventQueue(queue);
        mServerPtr->ClientDisconnected();
        return status;
    }

    grpc::Status Reset(grpc::ServerContext * /* aContext */,
                       const nexus::ResetRequest * /* aRequest */,
                       nexus::ResetResponse * /* aResponse */) override
    {
        Log("Reset requested. Signaling simulation loop to restart...");

        Simulation::Get().RequestRestart();

        return grpc::Status::OK;
    }

    grpc::Status SetSpeed(grpc::ServerContext * /* aContext */,
                          const nexus::SetSpeedRequest *aRequest,
                          nexus::SetSpeedResponse * /* aResponse */) override
    {
        float speedFactor = aRequest->speed_factor();

        if (speedFactor < 0.0f)
        {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Speed factor cannot be negative");
        }

        {
            std::lock_guard<Simulation> lock(Simulation::Get());
            Simulation::Get().SetSpeedFactor(speedFactor);
        }

        return grpc::Status::OK;
    }

    grpc::Status SetNodeState(grpc::ServerContext * /* aContext */,
                              const nexus::SetNodeStateRequest *aRequest,
                              nexus::SetNodeStateResponse * /* aResponse */) override
    {
        std::lock_guard<Simulation> lock(Simulation::Get());

        Log("SetNodeState: Node %d -> %s", aRequest->node_id(), aRequest->enabled() ? "ENABLE" : "DISABLE");
        Core::Get().SetNodeEnabled(aRequest->node_id(), aRequest->enabled());

        return grpc::Status::OK;
    }

    grpc::Status SetNodePosition(grpc::ServerContext * /* aContext */,
                                 const nexus::SetNodePositionRequest *aRequest,
                                 nexus::SetNodePositionResponse * /* aResponse */) override
    {
        grpc::Status                status = grpc::Status::OK;
        std::lock_guard<Simulation> lock(Simulation::Get());

        Node *node = Core::Get().FindNodeById(aRequest->node_id());

        VerifyOrExit(node != nullptr, status = grpc::Status(grpc::StatusCode::NOT_FOUND, "Node not found"));

        node->SetPosition(aRequest->x(), aRequest->y());
        mServerPtr->OnNodeStateChanged(node);

    exit:
        return status;
    }

    grpc::Status CreateNode(grpc::ServerContext * /* aContext */,
                            const nexus::CreateNodeRequest *aRequest,
                            nexus::CreateNodeResponse      *aResponse) override
    {
        std::lock_guard<Simulation> lock(Simulation::Get());

        Log("CreateNode: x=%f, y=%f", aRequest->x(), aRequest->y());
        Node &node = Core::Get().CreateNode();
        node.SetPosition(aRequest->x(), aRequest->y());
        mServerPtr->OnNodeStateChanged(&node);
        aResponse->set_node_id(node.GetId());
        Log("CreateNode: done, id=%u", node.GetId());

        return grpc::Status::OK;
    }

    grpc::Status FormNetwork(grpc::ServerContext * /* aContext */,
                             const nexus::FormNetworkRequest *aRequest,
                             nexus::FormNetworkResponse * /* aResponse */) override
    {
        grpc::Status                status = grpc::Status::OK;
        std::lock_guard<Simulation> lock(Simulation::Get());

        Node *node = Core::Get().FindNodeById(aRequest->node_id());

        VerifyOrExit(node != nullptr, status = grpc::Status(grpc::StatusCode::NOT_FOUND, "Node not found"));

        node->Form();

    exit:
        return status;
    }

    grpc::Status JoinNetwork(grpc::ServerContext * /* aContext */,
                             const nexus::JoinNetworkRequest *aRequest,
                             nexus::JoinNetworkResponse * /* aResponse */) override
    {
        grpc::Status                status = grpc::Status::OK;
        std::lock_guard<Simulation> lock(Simulation::Get());

        Node *joiner = Core::Get().FindNodeById(aRequest->node_id());
        Node *target = Core::Get().FindNodeById(aRequest->target_node_id());

        VerifyOrExit(joiner != nullptr && target != nullptr,
                     status = grpc::Status(grpc::StatusCode::NOT_FOUND, "Node not found"));

        {
            static constexpr Node::JoinMode kJoinModes[] = {
                Node::kAsFtd, // JOIN_MODE_FTD
                Node::kAsMed, // JOIN_MODE_MED
                Node::kAsSed, // JOIN_MODE_SED
                Node::kAsFed, // JOIN_MODE_FED
            };

            VerifyOrExit(static_cast<size_t>(aRequest->mode()) < GetArrayLength(kJoinModes),
                         status = grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid join mode"));

            Node::JoinMode mode = kJoinModes[aRequest->mode()];

            joiner->Join(*target, mode);
        }

    exit:
        return status;
    }

    grpc::Status GetRadioParameters(grpc::ServerContext * /* aContext */,
                                    const nexus::GetRadioParametersRequest * /* aRequest */,
                                    nexus::GetRadioParametersResponse *aResponse) override
    {
        aResponse->set_path_loss_constant(RadioModel::kPathLossConstant);
        aResponse->set_path_loss_exponent(RadioModel::kPathLossExponent);
        aResponse->set_radio_sensitivity(Radio::kRadioSensitivity);
        aResponse->set_mle_link_request_margin_min(OPENTHREAD_CONFIG_MLE_LINK_REQUEST_MARGIN_MIN);

        return grpc::Status::OK;
    }

private:
    GrpcServer *mServerPtr;
};

GrpcServer::GrpcServer(const char *aExecutablePath)
    : mRunning(true)
    , mClientCount(0)
    , mExecutablePath(aExecutablePath != nullptr ? aExecutablePath : "")
{
    mService = std::make_unique<ServiceImpl>(this);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(kDefaultGrpcServerAddress, grpc::InsecureServerCredentials());
    builder.RegisterService(mService.get());

    mServer = builder.BuildAndStart();

    if (mServer)
    {
        Log("gRPC Server started at %s", kDefaultGrpcServerAddress);
        mThread = std::thread([this]() { mServer->Wait(); });
        {
            std::lock_guard<Simulation> lock(Simulation::Get());
            Core::Get().AddObserver(*this);
        }
    }
    else
    {
        Log("Failed to start gRPC server!");
    }
}

GrpcServer::~GrpcServer()
{
    {
        std::lock_guard<Simulation> lock(Simulation::Get());
        Core::Get().RemoveObserver(*this);
    }
    mRunning = false;
    mCv.notify_all();
    if (mServer)
    {
        mServer->Shutdown();
    }
    if (mThread.joinable())
    {
        mThread.join();
    }
}

void GrpcServer::ClientConnected(void)
{
    mClientCount++;
    Log("UI connected!");
}

void GrpcServer::ClientDisconnected(void)
{
    if (mClientCount.fetch_sub(1) == 1)
    {
        Log("All UI clients disconnected.");
    }
}

void GrpcServer::PushEvent(const nexus::SimulationEvent &aEvent, std::queue<nexus::SimulationEvent> *aTargetQueue)
{
    std::lock_guard<std::mutex> lock(mMutex);

    if (aTargetQueue != nullptr)
    {
        if (aTargetQueue->size() >= kMaxQueueSize)
        {
            aTargetQueue->pop();
        }
        aTargetQueue->push(aEvent);
    }
    else
    {
        for (auto &queue : mClientQueues)
        {
            if (queue.size() >= kMaxQueueSize)
            {
                queue.pop();
            }
            queue.push(aEvent);
        }
    }
    mCv.notify_all();
}

bool GrpcServer::PopEvent(std::queue<nexus::SimulationEvent> *aQueue,
                          nexus::SimulationEvent             &aEvent,
                          uint32_t                            aTimeoutMs)
{
    bool                         found = false;
    std::unique_lock<std::mutex> lock(mMutex);

    VerifyOrExit(mCv.wait_for(lock, std::chrono::milliseconds(aTimeoutMs),
                              [this, aQueue] { return !aQueue->empty() || !mRunning; }));

    VerifyOrExit(mRunning || !aQueue->empty());

    aEvent = aQueue->front();
    aQueue->pop();
    found = true;

exit:
    return found;
}

std::queue<nexus::SimulationEvent> *GrpcServer::AddEventQueue(void)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mClientQueues.emplace_back();
    return &mClientQueues.back();
}

void GrpcServer::RemoveEventQueue(std::queue<nexus::SimulationEvent> *aQueue)
{
    std::lock_guard<std::mutex> lock(mMutex);

    for (auto it = mClientQueues.begin(); it != mClientQueues.end(); ++it)
    {
        if (&(*it) == aQueue)
        {
            mClientQueues.erase(it);
            break;
        }
    }
}

void GrpcServer::ClearQueue(void)
{
    std::lock_guard<std::mutex> lock(mMutex);

    for (auto &queue : mClientQueues)
    {
        std::queue<nexus::SimulationEvent> empty;
        std::swap(queue, empty);
    }
}

GrpcServer::PacketInfo GrpcServer::GetPacketInfo(const Mac::Frame &aFrame)
{
    PacketInfo info;

    if (aFrame.IsAck())
    {
        info.mProtocol = "IEEE 802.15.4 ACK";
        info.mSummary  = "ACK";
    }
    else if (aFrame.GetType() == Mac::Frame::kTypeData)
    {
        info.mProtocol = "IEEE 802.15.4 Data";
        info.mSummary  = "Data";
    }
    else if (aFrame.IsMacCommand())
    {
        info.mProtocol = "IEEE 802.15.4 Command";
        info.mSummary  = "Command";
    }
    else
    {
        info.mProtocol = "IEEE 802.15.4";
        info.mSummary  = "Other";
    }

    if (aFrame.IsSequencePresent())
    {
        char buf[32];

        snprintf(buf, sizeof(buf), " (seq=%u)", aFrame.GetSequence());
        info.mSummary += buf;
    }

    return info;
}

void GrpcServer::PushPacketEvent(uint32_t                            aSenderId,
                                 uint32_t                            aDestinationId,
                                 const uint8_t                      *aData,
                                 uint16_t                            aLen,
                                 std::queue<nexus::SimulationEvent> *aTargetQueue)
{
    nexus::SimulationEvent event;
    otRadioFrame           radioFrame;

    InitEvent(event);

    auto *packetEvent = event.mutable_packet_captured();
    packetEvent->set_source_node_id(aSenderId);
    packetEvent->set_destination_node_id(aDestinationId);
    packetEvent->set_raw_payload(aData, aLen);

    ClearAllBytes(radioFrame);
    radioFrame.mPsdu   = const_cast<uint8_t *>(aData);
    radioFrame.mLength = aLen;

    {
        const Mac::Frame &frame = static_cast<const Mac::Frame &>(radioFrame);
        PacketInfo        info  = GetPacketInfo(frame);

        packetEvent->set_protocol(info.mProtocol);
        packetEvent->set_summary(info.mSummary);
    }

    PushEvent(event, aTargetQueue);
}

void GrpcServer::PushNodeEvent(Node *aNode, std::queue<nexus::SimulationEvent> *aTargetQueue)
{
    nexus::SimulationEvent event;
    InitEvent(event);

    auto *nodeEvent = event.mutable_node_update();
    nodeEvent->set_node_id(aNode->GetId());
    nodeEvent->set_role(GetEnhancedRole(aNode));
    nodeEvent->set_rloc16(aNode->Get<Mle::Mle>().GetRloc16());
    nodeEvent->set_x(aNode->GetPositionX());
    nodeEvent->set_y(aNode->GetPositionY());

    Log("PushNodeEvent: id=%u, role=%d, x=%f, y=%f", nodeEvent->node_id(), static_cast<int>(nodeEvent->role()),
        nodeEvent->x(), nodeEvent->y());

    PushEvent(event, aTargetQueue);
}

void GrpcServer::PushLinkUpdate(uint32_t                            aSrcId,
                                uint32_t                            aDstId,
                                bool                                aIsActive,
                                std::queue<nexus::SimulationEvent> *aTargetQueue)
{
    Log("PushLinkUpdate: src=%u, dst=%u, active=%d", aSrcId, aDstId, aIsActive);

    nexus::SimulationEvent event;
    InitEvent(event);

    auto *linkEvent = event.mutable_link_update();
    linkEvent->set_source_node_id(aSrcId);
    linkEvent->set_destination_node_id(aDstId);
    linkEvent->set_is_active(aIsActive);

    PushEvent(event, aTargetQueue);
}

void GrpcServer::OnHeartbeat(uint64_t aTimestampUs)
{
    nexus::SimulationEvent event;
    event.set_timestamp_us(aTimestampUs);
    event.mutable_heartbeat();
    PushEvent(event);
}

void GrpcServer::DumpState(std::queue<nexus::SimulationEvent> *aQueue)
{
    std::lock_guard<Simulation> lock(Simulation::Get());

    {
        nexus::SimulationEvent event;
        event.set_timestamp_us(Core::Get().GetNowMicro64());
        event.mutable_heartbeat();
        PushEvent(event, aQueue);
    }

    Log("DumpState: Starting sync...");

    // 1. Sync Nodes
    for (Node &node : Core::Get().GetNodes())
    {
        PushNodeEvent(&node, aQueue);
    }

    // 2. Sync Links
    for (Node &node : Core::Get().GetNodes())
    {
        otNeighborInfoIterator iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;
        Neighbor::Info         neighborInfo;

        while (node.Get<NeighborTable>().GetNextNeighborInfo(iterator, neighborInfo) == kErrorNone)
        {
            Node *neighborNode =
                Core::Get().GetNodes().FindMatching(static_cast<const Mac::ExtAddress &>(neighborInfo.mExtAddress));

            if (neighborNode != nullptr)
            {
                PushLinkUpdate(node.GetId(), neighborNode->GetId(), true, aQueue);
            }
        }
    }
}

void GrpcServer::OnNodeStateChanged(Node *aNode) { PushNodeEvent(aNode); }

void GrpcServer::OnLinkUpdate(uint32_t aSrcId, uint32_t aDstId, bool aIsActive)
{
    PushLinkUpdate(aSrcId, aDstId, aIsActive);
}

void GrpcServer::OnPacketEvent(uint32_t aSenderId, uint32_t aDestinationId, const uint8_t *aData, uint16_t aLen)
{
    PushPacketEvent(aSenderId, aDestinationId, aData, aLen);
}

void GrpcServer::OnClearEvents(void) { ClearQueue(); }

bool GrpcServer::IsConnected(void) const { return mClientCount > 0; }

} // namespace Nexus
} // namespace ot

#endif // OPENTHREAD_NEXUS_CONFIG_GRPC_ENABLE
