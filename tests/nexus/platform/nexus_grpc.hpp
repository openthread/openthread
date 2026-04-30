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

#ifndef OT_NEXUS_PLATFORM_NEXUS_GRPC_HPP_
#define OT_NEXUS_PLATFORM_NEXUS_GRPC_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_NEXUS_CONFIG_GRPC_ENABLE

#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "nexus_core.hpp"
#include "simulation.pb.h"

namespace grpc {
class Server;
} // namespace grpc

namespace ot {
namespace Nexus {

constexpr char kDefaultGrpcServerAddress[] = "127.0.0.1:50052";

class GrpcServer : public Observer
{
public:
    /**
     * Initializes the gRPC server.
     *
     * @param[in] aExecutablePath  The path to the executable (used for Reset RPC).
     */
    GrpcServer(const char *aExecutablePath = nullptr);

    /**
     * Destructor for the gRPC server.
     */
    ~GrpcServer() override;

    // Observer implementation
    void OnNodeStateChanged(Node *aNode) override;
    void OnLinkUpdate(uint32_t aSrcId, uint32_t aDstId, bool aIsActive) override;
    void OnPacketEvent(uint32_t aSenderId, uint32_t aDestinationId, const uint8_t *aData, uint16_t aLen) override;
    void OnClearEvents(void) override;
    void OnHeartbeat(uint64_t aTimestampUs) override;
    void DumpState(void) override { DumpState(nullptr); }
    void DumpState(std::queue<nexus::SimulationEvent> *aQueue);
    bool IsConnected(void) const override;

    /**
     * Pushes a simulation event to the connected clients.
     *
     * @param[in] aEvent        The event to push.
     * @param[in] aTargetQueue  An optional pointer to a specific client's event queue.
     */
    void PushEvent(const nexus::SimulationEvent &aEvent, std::queue<nexus::SimulationEvent> *aTargetQueue = nullptr);

    /**
     * Gets the executable path.
     *
     * @returns The executable path.
     */
    const std::string &GetExecutablePath(void) const { return mExecutablePath; }

    /**
     * Indicates whether or not the server is running.
     *
     * @retval TRUE   If the server is running.
     * @retval FALSE  If the server is not running.
     */
    bool IsRunning(void) const { return mRunning; }

private:
    static constexpr size_t kMaxQueueSize = 1000;

    class ServiceImpl;

    struct PacketInfo
    {
        const char *mProtocol;
        std::string mSummary;
    };

    static nexus::NodeRole GetEnhancedRole(Node *aNode);
    static PacketInfo      GetPacketInfo(const Mac::Frame &aFrame);

    /**
     * Handles a new client connection.
     */
    void ClientConnected(void);

    /**
     * Handles a client disconnection.
     */
    void ClientDisconnected(void);

    /**
     * Pops an event from the event queue for a specific client.
     *
     * @param[in]  aQueue      A pointer to the client's event queue.
     * @param[out] aEvent      A reference to store the popped event.
     * @param[in]  aTimeoutMs  The timeout in milliseconds to wait for an event.
     *
     * @returns TRUE if an event was popped, FALSE otherwise.
     */
    bool PopEvent(std::queue<nexus::SimulationEvent> *aQueue, nexus::SimulationEvent &aEvent, uint32_t aTimeoutMs);

    /**
     * Adds an event queue for a new client.
     *
     * @returns A pointer to the newly created event queue.
     */
    std::queue<nexus::SimulationEvent> *AddEventQueue(void);

    /**
     * Removes an event queue for a disconnected client.
     *
     * @param[in] aQueue  A pointer to the event queue to remove.
     */
    void RemoveEventQueue(std::queue<nexus::SimulationEvent> *aQueue);

    /**
     * Initializes a simulation event with common fields (like timestamp).
     *
     * @param[out] aEvent  A reference to the event to initialize.
     */
    void InitEvent(nexus::SimulationEvent &aEvent);

    /**
     * Pushes a node update event.
     *
     * @param[in] aNode         The node that was updated.
     * @param[in] aTargetQueue  An optional pointer to a specific client's event queue.
     */
    void PushNodeEvent(Node *aNode, std::queue<nexus::SimulationEvent> *aTargetQueue = nullptr);

    /**
     * Pushes a link update event.
     *
     * @param[in] aSrcId        The source node ID.
     * @param[in] aDstId        The destination node ID.
     * @param[in] aIsActive     TRUE if the link is active, FALSE otherwise.
     * @param[in] aTargetQueue  An optional pointer to a specific client's event queue.
     */
    void PushLinkUpdate(uint32_t                            aSrcId,
                        uint32_t                            aDstId,
                        bool                                aIsActive,
                        std::queue<nexus::SimulationEvent> *aTargetQueue = nullptr);

    /**
     * Pushes a packet capture event.
     *
     * @param[in] aSenderId       The sender node ID.
     * @param[in] aDestinationId  The destination node ID.
     * @param[in] aData           A pointer to the packet data.
     * @param[in] aLen            The length of the packet data.
     * @param[in] aTargetQueue    An optional pointer to a specific client's event queue.
     */
    void PushPacketEvent(uint32_t                            aSenderId,
                         uint32_t                            aDestinationId,
                         const uint8_t                      *aData,
                         uint16_t                            aLen,
                         std::queue<nexus::SimulationEvent> *aTargetQueue = nullptr);

    /**
     * Clears the event queue.
     */
    void ClearQueue(void);

    std::unique_ptr<ServiceImpl>                  mService;
    std::unique_ptr<grpc::Server>                 mServer;
    std::thread                                   mThread;
    std::mutex                                    mMutex;
    std::condition_variable                       mCv;
    std::list<std::queue<nexus::SimulationEvent>> mClientQueues;
    std::atomic<bool>                             mRunning;
    std::atomic<int>                              mClientCount;
    std::string                                   mExecutablePath;
};

} // namespace Nexus
} // namespace ot

#endif // OPENTHREAD_NEXUS_CONFIG_GRPC_ENABLE

#endif // OT_NEXUS_PLATFORM_NEXUS_GRPC_HPP_
