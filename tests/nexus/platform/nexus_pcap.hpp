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

#ifndef OT_NEXUS_PLATFORM_NEXUS_PCAP_HPP_
#define OT_NEXUS_PLATFORM_NEXUS_PCAP_HPP_

#include <cstdint>
#include <cstdio>

#include "nexus_infra_if.hpp"
#include "nexus_radio.hpp"
#include "lib/pcapng/pcapng.hpp"

namespace ot {
namespace Nexus {

class Pcap
{
public:
    Pcap(void);
    ~Pcap(void);

    /**
     * Opens a pcapng file.
     *
     * The file describes two interfaces: IEEE 802.15.4 (for radio frames) and Ethernet (for infrastructure interface
     * packets).
     *
     * @param[in] aFilename  The filename to open.
     */
    void Open(const char *aFilename);

    /**
     * Closes the pcapng file.
     */
    void Close(void);

    /**
     * Writes a radio frame to the pcapng file.
     *
     * @param[in] aFrame   The frame to write.
     * @param[in] aTimeUs  The timestamp in microseconds.
     * @param[in] aRssi    The RSSI in dBm, or `OT_RADIO_RSSI_INVALID` if unknown.
     * @param[in] aLqi     The LQI, or `OT_RADIO_LQI_NONE` if unknown.
     */
    void WriteFrame(const otRadioFrame &aFrame, uint64_t aTimeUs, int8_t aRssi, uint8_t aLqi);

    /**
     * Writes an infrastructure interface IPv6 packet as an Ethernet frame to the pcapng file.
     *
     * @param[in] aSrcAddr  The source link-layer address.
     * @param[in] aDstAddr  The destination link-layer address.
     * @param[in] aBuffer   The packet buffer to write.
     * @param[in] aLength   The packet length.
     * @param[in] aTimeUs   The timestamp in microseconds.
     */
    void WritePacket(const InfraIf::LinkLayerAddress &aSrcAddr,
                     const InfraIf::LinkLayerAddress &aDstAddr,
                     const uint8_t                   *aBuffer,
                     uint16_t                         aLength,
                     uint64_t                         aTimeUs);

private:
#ifndef __EMSCRIPTEN__
    // Interface IDs follow the order of the Interface Description Blocks written by `Open()`.
    static constexpr uint32_t kInterfaceIdIeee802154 = 0;
    static constexpr uint32_t kInterfaceIdEthernet   = 1;

    static constexpr uint16_t kEtherTypeIp6 = 0x86dd;

    void Write(const Pcapng::Encoder &aEncoder);

    FILE   *mFile;
    uint8_t mBlock[Pcapng::Encoder::kMaxEthernetFrameBlockSize];
#endif
};

} // namespace Nexus
} // namespace ot

#endif // OT_NEXUS_PLATFORM_NEXUS_PCAP_HPP_
