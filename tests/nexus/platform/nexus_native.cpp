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

#include <chrono>
#include <csignal>
#include <memory>
#include <thread>
#include <vector>

#include <unistd.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_sim.hpp"
#if OPENTHREAD_NEXUS_CONFIG_GRPC_ENABLE
#include "platform/nexus_grpc.hpp"
#endif
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static std::atomic<bool> sRunning(true);

static void HandleSignal(int aSignal)
{
    OT_UNUSED_VARIABLE(aSignal);
    sRunning = false;
}

static void WaitForPlay(Simulation &aSim)
{
    static constexpr uint32_t kWaitTimeoutMs      = 1000;
    static constexpr float    kPlaySpeedThreshold = 0.1f;

    Log("Waiting for Play command to start simulation...");

    while (sRunning)
    {
        float speedFactor;

        {
            std::lock_guard<Simulation> lock(aSim);
            speedFactor = aSim.GetSpeedFactor();
        }

        if (speedFactor >= kPlaySpeedThreshold)
        {
            break;
        }

        aSim.WaitSpeedChange(kWaitTimeoutMs);
    }

    if (sRunning)
    {
        Log("Play command received! Running simulation loop...");
    }
}

static void HandleSimulationStep(Core       &aNexus,
                                 Simulation &aSim,
                                 uint32_t    aAdvanceTimeMs,
                                 uint32_t   &aHeartbeatTimerRealTimeUs)
{
    std::lock_guard<Simulation> lock(aSim);
    float                       speedFactor;

    aNexus.AdvanceTime(aAdvanceTimeMs);
    speedFactor = aSim.GetSpeedFactor();

#if OPENTHREAD_NEXUS_CONFIG_GRPC_ENABLE
    if (speedFactor > 0.0f)
    {
        static constexpr uint32_t kHeartbeatSyncIntervalUs = 100000; // 100ms real-time

        uint64_t increment = static_cast<uint64_t>((static_cast<double>(aAdvanceTimeMs) * 1000.0) / speedFactor);
        uint64_t newTotal  = static_cast<uint64_t>(aHeartbeatTimerRealTimeUs) + increment;

        if (newTotal >= kHeartbeatSyncIntervalUs)
        {
            aNexus.NotifyHeartbeat();
            aHeartbeatTimerRealTimeUs = static_cast<uint32_t>(newTotal % kHeartbeatSyncIntervalUs);
        }
        else
        {
            aHeartbeatTimerRealTimeUs = static_cast<uint32_t>(newTotal);
        }
    }
#else
    OT_UNUSED_VARIABLE(aHeartbeatTimerRealTimeUs);
    OT_UNUSED_VARIABLE(speedFactor);
#endif
}

static void SleepUntilIntendedTime(std::chrono::steady_clock::time_point aStartTime,
                                   uint32_t                              aAdvanceTimeMs,
                                   float                                 aSpeedFactor)
{
    std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();
    auto     elapsedUs     = std::chrono::duration_cast<std::chrono::microseconds>(endTime - aStartTime).count();
    uint64_t intendedSleep = static_cast<uint64_t>((aAdvanceTimeMs * 1000.0f) / aSpeedFactor);

    if (intendedSleep > static_cast<uint64_t>(elapsedUs))
    {
        std::this_thread::sleep_for(std::chrono::microseconds(intendedSleep - elapsedUs));
    }
}

void LiveDemo(int argc, char *argv[])
{
    static constexpr uint32_t kAdvanceTimeMs = 100;
    static constexpr uint32_t kIdleSleepMs   = 100;

    Core       nexus;
    Simulation sim;
#if OPENTHREAD_NEXUS_CONFIG_GRPC_ENABLE
    auto grpcServer = std::make_unique<GrpcServer>(argv[0]);
#endif

    signal(SIGINT, HandleSignal);
    signal(SIGTERM, HandleSignal);

    Log("================================================================================");
    Log("Starting OpenThread Nexus Simulator - Dynamic Topology");
    Log("================================================================================");

    WaitForPlay(sim);

    uint32_t heartbeatTimerRealTimeUs = 0;

    while (sRunning)
    {
        float                                 speedFactor;
        bool                                  isUiConnected;
        std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

        if (sim.IsRestartRequested())
        {
            break;
        }

        {
            std::lock_guard<Simulation> lock(sim);
            isUiConnected = nexus.IsUiConnected();
            speedFactor   = sim.GetSpeedFactor();
        }

        if (speedFactor <= 0.0f)
        {
            sim.WaitSpeedChange(kAdvanceTimeMs);
            continue;
        }

        if (isUiConnected)
        {
            HandleSimulationStep(nexus, sim, kAdvanceTimeMs, heartbeatTimerRealTimeUs);
            SleepUntilIntendedTime(startTime, kAdvanceTimeMs, speedFactor);
        }
        else
        {
            sim.WaitSpeedChange(kIdleSleepMs);
        }
    }

    if (sim.IsRestartRequested())
    {
        Log("Restarting process...");

#if OPENTHREAD_NEXUS_CONFIG_GRPC_ENABLE
        std::string execPath = grpcServer->GetExecutablePath();
#else
        std::string execPath = argv[0];
#endif
        if (execPath.empty())
        {
            execPath = argv[0];
        }

        std::vector<char *> args;
        args.push_back(const_cast<char *>(execPath.data()));
        for (int i = 1; i < argc; i++)
        {
            args.push_back(argv[i]);
        }
        args.push_back(nullptr);
        execvp(args[0], args.data());

        // If execvp returns, it failed
        Log("execvp failed: %s", strerror(errno));
        _exit(1);
    }

    Log("Simulation terminated gracefully.");
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::LiveDemo(argc, argv);
    return 0;
}
