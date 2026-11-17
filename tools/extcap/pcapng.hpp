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

#ifndef OT_EXTCAP_PCAPNG_HPP_
#define OT_EXTCAP_PCAPNG_HPP_

#include <stdint.h>
#include <stdio.h>
#include <openthread/platform/radio.h>

namespace ot {
namespace Extcap {

/**
 * This class implements a PCAPNG file writer for OpenThread extcap sniffer.
 *
 */
class PcapngWriter
{
public:
    /**
     * Constructor.
     *
     */
    PcapngWriter(void);

    PcapngWriter(const PcapngWriter &)            = delete;
    PcapngWriter &operator=(const PcapngWriter &) = delete;

    /**
     * Destructor.
     *
     */
    ~PcapngWriter(void);

    /**
     * Opens a PCAPNG file or FIFO for writing.
     *
     * @param[in] aFilename  The path to the file or FIFO.
     *
     * @retval OT_ERROR_NONE    Successfully opened the file and wrote headers.
     * @retval OT_ERROR_FAILED  Failed to open the file or write headers.
     *
     */
    otError Open(const char *aFilename);

    /**
     * Closes the PCAPNG file.
     *
     */
    void Close(void);

    /**
     * Checks if the PCAPNG file is open.
     *
     * @retval true   The file is open.
     * @retval false  The file is not open.
     *
     */
    bool IsOpen(void) const { return mFile != nullptr; }

    /**
     * Writes a received IEEE 802.15.4 frame to the PCAPNG file.
     *
     * @param[in] aFrame   The received Radio Frame to write.
     * @param[in] aTimeUs  The capture timestamp in microseconds since the Unix epoch.
     *
     * @retval OT_ERROR_NONE          Successfully wrote the frame.
     * @retval OT_ERROR_INVALID_STATE The file is not open.
     * @retval OT_ERROR_FAILED        Failed to write to the file.
     *
     */
    otError WriteFrame(const otRadioFrame &aFrame, uint64_t aTimeUs);

private:
    otError Write(const uint8_t *aData, size_t aLength);

    static constexpr uint32_t kPcapngShbType            = 0x0a0d0d0a;
    static constexpr uint32_t kPcapngByteOrderMagic     = 0x1a2b3c4d;
    static constexpr uint16_t kPcapngVersionMajor       = 1;
    static constexpr uint16_t kPcapngVersionMinor       = 0;
    static constexpr uint32_t kPcapngIdbType            = 0x00000001;
    static constexpr uint32_t kPcapngEpbType            = 0x00000006;
    static constexpr uint32_t kPcapngSnapLen            = 65535;
    static constexpr uint32_t kPcapngLinkTypeIeee802154 = 283; // DLT_IEEE802_15_4_TAP

    static constexpr uint8_t kTapVersion = 0;

    static constexpr uint16_t kTapFcsType   = 0;
    static constexpr uint16_t kTapFcsLength = 1;
    static constexpr uint8_t  kTapFcsValue  = 1; // 16-bit CRC

    static constexpr uint16_t kTapChannelType   = 3;
    static constexpr uint16_t kTapChannelLength = 3;
    static constexpr uint8_t  kTapChannelPage   = 0;

    static constexpr uint16_t kTapRssiType   = 1;
    static constexpr uint16_t kTapRssiLength = 4;

    static constexpr uint16_t kTapLqiType   = 10;
    static constexpr uint16_t kTapLqiLength = 1;

    FILE *mFile;
};

} // namespace Extcap
} // namespace ot

#endif // OT_EXTCAP_PCAPNG_HPP_
