/*
 *  Copyright (c) 2026, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Fuzz target for the SRP server's DNS-update message parser
 *  (Srp::Server::ProcessMessage -> ProcessDnsUpdate -> Process{Zone,Update,Additional}Section).
 *  A second node (an ordinary mesh peer = attacker) sends crafted DNS-update bytes over UDP
 *  to the leader/border-router's SRP server port. No existing OSS-Fuzz target reaches this parser.
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
    Message       *message         = nullptr;

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

    // Border-router leader running the SRP server (matches the working ip6 harness setup).
    Node &server = nexus.CreateNode();
    SuccessOrQuit(server.GetInstance().SetLogLevel(kLogLevelNone));
    server.GetInstance().Get<Srp::Server>().SetAutoEnableMode(true);
    server.GetInstance().Get<BorderRouter::InfraIf>().Init(/* aInfraIfIndex */ 1, /* aIsRunning */ true);
    SuccessOrQuit(server.GetInstance().Get<BorderRouter::RoutingManager>().SetEnabled(true));

    server.Form();
    nexus.AdvanceTime(60 * 1000);
    VerifyOrQuit(server.Get<Mle::Mle>().IsLeader());

    if (server.Get<Srp::Server>().GetState() != Srp::Server::kStateRunning)
    {
        return 0;
    }

    // A peer node joins the mesh (the attacker).
    Node &sender = nexus.CreateNode();
    sender.Join(server);
    nexus.AdvanceTime(60 * 1000);

    if (!sender.Get<Mle::Mle>().IsAttached())
    {
        return 0;
    }

    {
        uint16_t         port = server.Get<Srp::Server>().GetPort();
        Ip6::Address     dest = server.Get<Mle::Mle>().GetMeshLocalEid();
        Ip6::Udp::Socket socket(sender.GetInstance(), &FuzzUdpNoop, nullptr);
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

        IgnoreError(socket.Close());
    }

exit:
    if (message != nullptr)
    {
        message->Free();
    }
    return 0;
}

} // namespace Nexus
} // namespace ot
