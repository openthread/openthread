/*
 *  MeshCoP Dataset TLV parser fuzz harness (added for security research).
 *
 *  Rationale: OSS-Fuzz for OpenThread only covers ip6/icmp6/mdns/trel/radio/cli.
 *  The MeshCoP operational-dataset TLV parser (Dataset::SetFrom -> ValidateTlvs
 *  -> ConvertTo) is attacker-reachable via MGMT_ACTIVE_SET / MGMT_PENDING_SET
 *  (commissioner -> leader) and is NOT covered. This harness models the real
 *  receive path in dataset_manager_ftd.cpp:62-63 (SetFrom then ValidateTlvs),
 *  and only calls ConvertTo on a validated dataset, matching the production
 *  contract.
 */

#include <stddef.h>
#include <stdint.h>

#include "platform/nexus_core.hpp"

#include "meshcop/dataset.hpp"

namespace ot {
namespace Nexus {

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0 || size > ot::MeshCoP::Dataset::kMaxLength)
    {
        return 0;
    }

    // Dataset::SetFrom() records TimerMilli::GetNow(), which resolves through
    // Core::Get(); a Core is therefore required. CreateNode() (a full simulated
    // node) is not, as Dataset TLV parsing is instance-independent.
    Core nexus;
    OT_UNUSED_VARIABLE(nexus);

    ot::MeshCoP::Dataset dataset;

    if (dataset.SetFrom(data, static_cast<uint8_t>(size)) != kErrorNone)
    {
        return 0;
    }

    // Faithful to dataset_manager_ftd.cpp: parse then validate.
    if (dataset.ValidateTlvs() == kErrorNone)
    {
        // ConvertTo is only invoked on validated datasets in production.
        ot::MeshCoP::Dataset::Info info;
        dataset.ConvertTo(info);

        ot::MeshCoP::Timestamp ts;
        IgnoreError(dataset.ReadTimestamp(ot::MeshCoP::Dataset::kActive, ts));
        IgnoreError(dataset.ReadTimestamp(ot::MeshCoP::Dataset::kPending, ts));
    }

    return 0;
}

} // namespace Nexus
} // namespace ot
