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

/**
 * @file
 *   This file defines the WebAssembly bindings for the Nexus simulator.
 */

#include "openthread-core-config.h"

#include <deque>
#include <string>
#include <vector>

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <openthread/thread.h>
#include <openthread/thread_ftd.h>

#include "nexus_core.hpp"
#include "nexus_node.hpp"
#include "mac/mac.hpp"
#include "thread/mle.hpp"
#include "thread/neighbor_table.hpp"

namespace ot {
namespace Nexus {

using namespace emscripten;

/**
 * This class manages simulation events and exposes them to the WASM environment.
 *
 * It implements the Observer interface to capture events from the simulator core.
 */
class WasmManager : public Observer
{
public:
    static constexpr char kEventNodeStateChanged[] = "node_state_changed";
    static constexpr char kEventLinkUpdate[]       = "link_update";
    static constexpr char kEventPacketEvent[]      = "packet_event";
    static constexpr char kEventHeartbeat[]        = "heartbeat";

    /**
     * Represents a simulation event to be consumed by JavaScript.
     */
    struct Event
    {
        std::string mType;
        val         mData;
    };

    /**
     * Returns the singleton instance of WasmManager.
     *
     * @returns The WasmManager instance.
     */
    static WasmManager &Get(void)
    {
        static WasmManager sManager;
        return sManager;
    }

    /**
     * Initializes the manager and attaches it as an observer to the core.
     *
     * This method is idempotent and ensures the manager is registered as a simulation observer.
     */
    void Init(void)
    {
        if (!mInitialized)
        {
            Core::Get().AddObserver(*this);
            mInitialized = true;
        }
    }

    /**
     * Polls the next simulation event from the internal queue.
     *
     * The returned event object has the following structure in JavaScript:
     * {
     *   "type": string (e.g., "node_state_changed", "link_update", "packet_event"),
     *   "data": object (type-specific payload)
     * }
     *
     * @returns The event object or val::null() if the queue is empty.
     */
    val PollEvent(void)
    {
        val eventVal = val::null();

        if (!mEventQueue.empty())
        {
            const Event &event = mEventQueue.front();

            eventVal = val::object();
            eventVal.set("type", event.mType);
            eventVal.set("data", event.mData);
            mEventQueue.pop_front();
        }

        return eventVal;
    }

    // Observer overrides
    void OnNodeStateChanged(Node *aNode) override
    {
        val data = val::object();

        data.set("id", aNode->GetId());
        data.set("role", aNode->GetExtendedRoleString());
        data.set("x", aNode->GetPositionX());
        data.set("y", aNode->GetPositionY());
        data.set("rloc16", aNode->Get<Mle::Mle>().GetRloc16());
        data.set("extAddress", aNode->Get<Mac::Mac>().GetExtAddress().ToString().AsCString());

        PushEvent(kEventNodeStateChanged, data);
    }

    void OnLinkUpdate(uint32_t aSrcId, uint32_t aDstId, bool aIsActive) override
    {
        val data = val::object();

        data.set("srcId", aSrcId);
        data.set("dstId", aDstId);
        data.set("isActive", aIsActive);

        PushEvent(kEventLinkUpdate, data);
    }

    void OnPacketEvent(uint32_t aSrcId, uint32_t aDstId, const uint8_t *aFrame, uint16_t aLength) override
    {
        val data = val::object();

        data.set("srcId", aSrcId);
        data.set("dstId", aDstId);
        data.set("length", aLength);
        data.set("frame", val(typed_memory_view(aLength, aFrame)).call<val>("slice"));

        PushEvent(kEventPacketEvent, data);
    }

    void OnHeartbeat(uint64_t aTimestampUs) override
    {
        val data = val::object();

        data.set("timestampUs", aTimestampUs);

        PushEvent(kEventHeartbeat, data);
    }

    void OnClearEvents(void) override { mEventQueue.clear(); }

    /**
     * WasmManager does not participate in state dumping as events are streamed in real-time.
     */
    void DumpState(void) override {}

    bool IsConnected(void) const override { return true; }

private:
    static constexpr size_t kMaxQueueSize = 1000;

    void PushEvent(const std::string &aType, val aData)
    {
        if (mEventQueue.size() >= kMaxQueueSize)
        {
            mEventQueue.pop_front();
        }
        mEventQueue.push_back({aType, aData});
    }

    std::deque<Event> mEventQueue; // Internal event queue (should be polled regularly by JS to prevent growth)
    bool              mInitialized = false;
};

// The global simulator core instance.
Core gWasmCore;

} // namespace Nexus
} // namespace ot

// Embind definitions
EMSCRIPTEN_BINDINGS(nexus_simulator)
{
    using namespace ot::Nexus;

    // Initialize WasmManager during module binding
    WasmManager::Get().Init();

    enum_<Node::JoinMode>("JoinMode")
        .value("AsFtd", Node::kAsFtd)
        .value("AsFed", Node::kAsFed)
        .value("AsMed", Node::kAsMed)
        .value("AsSed", Node::kAsSed)
        .value("AsSedWithFullNetData", Node::kAsSedWithFullNetData);

    function("pollEvent", optional_override([]() -> val { return WasmManager::Get().PollEvent(); }));

    function("getNow", optional_override([]() -> uint32_t { return Core::Get().GetNow().GetValue(); }));

    function("stepSimulation", optional_override([](uint32_t aElapsedMs) { Core::Get().AdvanceTime(aElapsedMs); }));

    function("createNode", optional_override([](float aX, float aY) -> uint32_t {
                 Node &node = Core::Get().CreateNode();
                 node.SetPosition(aX, aY);
                 WasmManager::Get().OnNodeStateChanged(&node);
                 return node.GetId();
             }));

    function("setNodePosition", optional_override([](uint32_t aNodeId, float aX, float aY) {
                 Node *node = Core::Get().FindNodeById(aNodeId);

                 if (node != nullptr)
                 {
                     node->SetPosition(aX, aY);
                 }
             }));

    function("getNodeId", optional_override([](std::string aExtAddress) -> uint32_t {
                 ot::Mac::ExtAddress extAddr;
                 uint32_t            id = kInvalidNodeId;

                 if (extAddr.FromString(aExtAddress.c_str()) == ot::kErrorNone)
                 {
                     Node *node = Core::Get().GetNodes().FindMatching(extAddr);
                     if (node != nullptr)
                     {
                         id = node->GetId();
                     }
                 }
                 return id;
             }));

    function("setNodeEnabled",
             optional_override([](uint32_t aNodeId, bool aEnabled) { Core::Get().SetNodeEnabled(aNodeId, aEnabled); }));

    function("formNetwork", optional_override([](uint32_t aNodeId) {
                 Node *node = Core::Get().FindNodeById(aNodeId);

                 if (node != nullptr)
                 {
                     node->Form();
                 }
             }));

    function("joinNetwork", optional_override([](uint32_t aNodeId, uint32_t aTargetNodeId, Node::JoinMode aJoinMode) {
                 Core &core   = Core::Get();
                 Node *node   = core.FindNodeById(aNodeId);
                 Node *target = core.FindNodeById(aTargetNodeId);

                 if (node != nullptr && target != nullptr)
                 {
                     node->Join(*target, aJoinMode);
                 }
             }));
}
