/*
 *  IPv6 Neighbor Discovery Router Advertisement option parser fuzz harness
 *  (added for security research).
 *
 *  Rationale: OSS-Fuzz for OpenThread covers only ip6/icmp6/mdns/trel/radio/cli.
 *  A Border Router parses Router Advertisements received on the infrastructure
 *  link (`RoutingManager`), iterating variable-length ND options (PIO, RIO,
 *  RA-flags, RDNSS). This option parser is attacker-reachable from the infra
 *  link and is NOT covered by OSS-Fuzz. This harness mirrors the real receive
 *  contract in routing_manager.cpp: construct `RouterAdvert::RxMessage`, check
 *  `IsValid()`, then iterate options and validate the typed options.
 */

#include <stddef.h>
#include <stdint.h>

#include "net/nd6.hpp"

namespace ot {
namespace Nexus {

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0 || size > 1500)
    {
        return 0;
    }

    ot::Ip6::Nd::Icmp6Packet packet;
    packet.Init(data, static_cast<uint16_t>(size));

    ot::Ip6::Nd::RouterAdvert::RxMessage raMsg(packet);

    if (!raMsg.IsValid())
    {
        return 0;
    }

    for (const ot::Ip6::Nd::Option &option : raMsg)
    {
        switch (option.GetType())
        {
        case ot::Ip6::Nd::Option::kTypePrefixInfo:
            OT_UNUSED_VARIABLE(static_cast<const ot::Ip6::Nd::PrefixInfoOption &>(option).IsValid());
            break;
        case ot::Ip6::Nd::Option::kTypeRouteInfo:
            OT_UNUSED_VARIABLE(static_cast<const ot::Ip6::Nd::RouteInfoOption &>(option).IsValid());
            break;
        case ot::Ip6::Nd::Option::kTypeRaFlagsExtension:
            OT_UNUSED_VARIABLE(static_cast<const ot::Ip6::Nd::RaFlagsExtOption &>(option).IsValid());
            break;
        case ot::Ip6::Nd::Option::kTypeNat64Prefix:
            OT_UNUSED_VARIABLE(static_cast<const ot::Ip6::Nd::Nat64PrefixOption &>(option).IsValid());
            break;
        case ot::Ip6::Nd::Option::kTypeRecursiveDnsServer:
            OT_UNUSED_VARIABLE(static_cast<const ot::Ip6::Nd::RecursiveDnsServerOption &>(option).IsValid());
            break;
        default:
            break;
        }
    }

    return 0;
}

} // namespace Nexus
} // namespace ot
