/*
 *  6LoWPAN IPHC header decompression fuzz harness (added for security research).
 *
 *  Rationale: OSS-Fuzz for OpenThread covers only ip6/icmp6/mdns/trel/radio/cli.
 *  6LoWPAN decompression (`Lowpan::Lowpan::Decompress`) reconstructs an IPv6
 *  header from a compressed, attacker-supplied mesh frame. It runs after MAC
 *  security, so the radio fuzz target cannot reach it, and it is not otherwise
 *  covered. This harness mirrors the real receive path in
 *  MeshForwarder::FrameToMessage: allocate an IPv6 message, then call
 *  Decompress() with attacker-controlled MAC addresses, frame data and
 *  datagram length.
 */

#include <stddef.h>
#include <stdint.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include "common/frame_data.hpp"
#include "mac/mac_types.hpp"
#include "thread/lowpan.hpp"

namespace ot {
namespace Nexus {

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // 8 bytes source EUI-64 + 8 bytes dest EUI-64 + 2 bytes datagram length + frame.
    if (size < 19 || size > 1500)
    {
        return 0;
    }

    Core  nexus;
    Node &node = nexus.CreateNode();

    Mac::Addresses macAddrs;
    macAddrs.mSource.SetExtended(&data[0]);
    macAddrs.mDestination.SetExtended(&data[8]);

    uint16_t datagramLength = static_cast<uint16_t>((data[16] << 8) | data[17]);

    FrameData frameData;
    frameData.Init(&data[18], static_cast<uint16_t>(size - 18));

    Message *message = node.GetInstance().Get<MessagePool>().Allocate(Message::kTypeIp6);

    if (message != nullptr)
    {
        IgnoreError(node.GetInstance().Get<Lowpan::Lowpan>().Decompress(*message, macAddrs, frameData, datagramLength));
        message->Free();
    }

    return 0;
}

} // namespace Nexus
} // namespace ot
