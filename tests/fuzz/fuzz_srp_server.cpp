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
 *   Fuzz target for the SRP server's DNS-update message parser
 *   (Srp::Server::ProcessMessage -> ProcessDnsUpdate -> Process{Zone,Update,Additional}Section).
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static void FuzzUdpNoop(void *, otMessage *, const otMessageInfo *) {}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const uint16_t kMaxMessageSize = 2048;
    unsigned int   seed;
    Message       *message = nullptr;

    if (size < sizeof(seed))
    {
        return 0;
    }
    if (size > sizeof(seed) + kMaxMessageSize)
    {
        return 0;
    }

    memcpy(&seed, data, sizeof(seed));
    srand(seed);
    data += sizeof(seed);
    size -= sizeof(seed);

    Core nexus;

    Node &server = nexus.CreateNode();
    SuccessOrQuit(server.GetInstance().SetLogLevel(kLogLevelNone));
    server.GetInstance().Get<Srp::Server>().SetAutoEnableMode(true);
    server.GetInstance().Get<BorderRouter::InfraIf>().Init(/* aInfraIfIndex */ 1, /* aIsRunning */ true);
    SuccessOrQuit(server.GetInstance().Get<BorderRouter::RoutingManager>().SetEnabled(true));

    server.Form();
    nexus.AdvanceTime(60 * 1000);
    VerifyOrQuit(server.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(server.Get<Srp::Server>().GetState() == Srp::Server::kStateRunning);

    {
        uint16_t         port = server.Get<Srp::Server>().GetPort();
        Ip6::Address     dest = server.Get<Mle::Mle>().GetMeshLocalEid();
        Ip6::Udp::Socket socket(server.GetInstance(), &FuzzUdpNoop, nullptr);
        Ip6::MessageInfo info;

        SuccessOrExit(socket.Open(Ip6::kNetifThreadInternal));

        message = socket.NewMessage();
        VerifyOrExit(message != nullptr);

        SuccessOrExit(message->AppendBytes(data, static_cast<uint16_t>(size)));

        info.SetPeerAddr(dest);
        info.SetPeerPort(port);

        if (socket.SendTo(*message, info) == kErrorNone)
        {
            message = nullptr;
        }

        nexus.AdvanceTime(5 * 1000);

    exit:
        IgnoreError(socket.Close());
    }

    if (message != nullptr)
    {
        message->Free();
    }
    return 0;
}

} // namespace Nexus
} // namespace ot
