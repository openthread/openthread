/*
 *  Copyright (c) 2026, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Fuzz target for the unicast DNS-SD server's query parser
 *  (Dns::ServiceDiscovery::Server::HandleUdpReceive and the DNS query/name/question
 *  decoding it performs). The DNS-SD server answers DNS queries received over UDP
 *  on port 53 from nodes on the Thread mesh, i.e. attacker-influenceable input.
 *  No existing fuzz target delivers crafted DNS query bytes to this server's UDP port.
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

    Core  nexus;
    Node &node = nexus.CreateNode();

    SuccessOrQuit(node.GetInstance().SetLogLevel(kLogLevelNone));

    node.Form();
    nexus.AdvanceTime(60 * 1000);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());

    // Explicitly enable the SRP server and start the unicast DNS-SD server so that the
    // UDP socket on port 53 is bound and `HandleUdpReceive` (the query parser) is reachable.
    node.Get<Srp::Server>().SetEnabled(true);
    SuccessOrQuit(node.Get<Dns::ServiceDiscovery::Server>().Start());
    nexus.AdvanceTime(1 * 1000);

    {
        uint16_t         port = Dns::ServiceDiscovery::Server::kPort;
        Ip6::Address     dest = node.Get<Mle::Mle>().GetMeshLocalEid();
        Ip6::Udp::Socket socket(node.GetInstance(), &FuzzUdpNoop, nullptr);
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

        nexus.AdvanceTime(2 * 1000);

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
