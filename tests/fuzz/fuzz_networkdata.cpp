/*
 *  Thread Network Data TLV parser fuzz harness (added for security research).
 *
 *  Rationale: OSS-Fuzz for OpenThread covers only ip6/icmp6/mdns/trel/radio/cli.
 *  Thread Network Data is distributed by the Leader to every node and parsed
 *  (ValidateTlvs + iteration over on-mesh prefixes, external routes, services,
 *  6LoWPAN contexts) on receipt. This recursive sub-TLV parser is attacker-
 *  reachable and NOT covered by OSS-Fuzz. The `NetworkData(Instance&, tlvs, len)`
 *  constructor is a public buffer view, so this harness feeds fuzz bytes to the
 *  real validation/iteration code paths.
 */

#include <stddef.h>
#include <stdint.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include "thread/network_data.hpp"

namespace ot {
namespace Nexus {

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0 || size > ot::NetworkData::NetworkData::kMaxSize)
    {
        return 0;
    }

    Core  nexus;
    Node &node = nexus.CreateNode();

    ot::NetworkData::NetworkData netData(node.GetInstance(), data, static_cast<uint8_t>(size));

    IgnoreError(netData.ValidateTlvs());

    {
        ot::NetworkData::Iterator           iter = ot::NetworkData::kIteratorInit;
        ot::NetworkData::OnMeshPrefixConfig cfg;
        while (netData.GetNext(iter, cfg) == kErrorNone)
        {
        }
    }
    {
        ot::NetworkData::Iterator            iter = ot::NetworkData::kIteratorInit;
        ot::NetworkData::ExternalRouteConfig cfg;
        while (netData.GetNext(iter, cfg) == kErrorNone)
        {
        }
    }
    {
        ot::NetworkData::Iterator      iter = ot::NetworkData::kIteratorInit;
        ot::NetworkData::ServiceConfig cfg;
        while (netData.GetNext(iter, cfg) == kErrorNone)
        {
        }
    }
    {
        ot::NetworkData::Iterator          iter = ot::NetworkData::kIteratorInit;
        ot::NetworkData::LowpanContextInfo cfg;
        while (netData.GetNext(iter, cfg) == kErrorNone)
        {
        }
    }

    return 0;
}

} // namespace Nexus
} // namespace ot
