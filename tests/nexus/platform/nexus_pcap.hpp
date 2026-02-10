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

#include <openthread/platform/radio.h>

namespace ot {
namespace Nexus {

class Pcap
{
public:
    Pcap(void);
    ~Pcap(void);

    /**
     * Opens a pcap file.
     *
     * @param[in] aFilename  The filename to open.
     */
    void Open(const char *aFilename);

    /**
     * Closes the pcap file.
     */
    void Close(void);

    /**
     * Writes a frame to the pcap file.
     *
     * @param[in] aFrame   The frame to write.
     * @param[in] aTimeUs  The timestamp in microseconds.
     */
    void WriteFrame(const otRadioFrame &aFrame, uint64_t aTimeUs);

private:
    static constexpr uint32_t kPcapMagicNumber  = 0xa1b2c3d4;
    static constexpr uint16_t kPcapVersionMajor = 2;
    static constexpr uint16_t kPcapVersionMinor = 4;
    static constexpr uint32_t kPcapSnapLen      = 65535;
    static constexpr uint32_t kPcapDlt154Tap    = 283;

    static constexpr uint8_t kTapVersion = 0;

    static constexpr uint16_t kTapFcsType   = 0;
    static constexpr uint16_t kTapFcsLength = 1;
    static constexpr uint8_t  kTapFcsValue  = 1; // 16-bit CRC

    static constexpr uint16_t kTapChannelType   = 3;
    static constexpr uint16_t kTapChannelLength = 3;
    static constexpr uint8_t  kTapChannelPage   = 0;

    FILE *mFile;
};

} // namespace Nexus
} // namespace ot

#endif // OT_NEXUS_PLATFORM_NEXUS_PCAP_HPP_
