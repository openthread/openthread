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

#include <string.h>

#include "common/encoding.hpp"
#include "lib/pcapng/pcapng.hpp"

#include "test_platform.h"
#include "test_util.hpp"

namespace ot {
namespace Pcapng {

// Offsets within a block.
constexpr uint32_t kBlockTotalLengthOffset  = 4;
constexpr uint32_t kIdbLinkTypeOffset       = 8;
constexpr uint32_t kEpbInterfaceIdOffset    = 8;
constexpr uint32_t kEpbTimestampHighOffset  = 12;
constexpr uint32_t kEpbTimestampLowOffset   = 16;
constexpr uint32_t kEpbCapturedLengthOffset = 20;
constexpr uint32_t kEpbPacketOffset         = 28;

// Offsets within the IEEE 802.15.4 TAP header, whose TLVs are ordered by type: FCS, RSSI, Channel, LQI.
constexpr uint32_t kTapLengthOffset   = 2;
constexpr uint32_t kTapRssiTlvOffset  = 12;
constexpr uint32_t kTapLqiTlvOffset   = 28;
constexpr uint32_t kTapTlvValueOffset = 4;
constexpr uint16_t kTapRssiTlvType    = 1;
constexpr uint16_t kTapLqiTlvType     = 10;

constexpr uint16_t kTapSizeWithoutRssiLqi = 4 + 2 * 8; // TAP header, FCS and Channel TLVs
constexpr uint16_t kTapSizeWithRssiLqi    = 4 + 4 * 8; // TAP header, FCS, RSSI, Channel and LQI TLVs

constexpr uint32_t kEthernetEtherTypeOffset = 12;
constexpr uint16_t kEtherTypeIp6            = 0x86dd;

constexpr uint8_t  kPsdu[]  = {'1', '2', '3', '4', '5', '6', '7', '8', '9', 0, 0}; // FCS left zero
constexpr uint16_t kPsduFcs = 0x2189; // CRC-16/KERMIT check value of "123456789"
constexpr int8_t   kRssi    = -50;
constexpr uint8_t  kLqi     = 200;

void VerifyBlockTotalLength(const uint8_t *aBlock, uint32_t aLength)
{
    VerifyOrQuit(LittleEndian::ReadUint32(aBlock + kBlockTotalLengthOffset) == aLength);
    VerifyOrQuit(LittleEndian::ReadUint32(aBlock + aLength - sizeof(uint32_t)) == aLength);
}

void TestSectionHeaderAndInterfaceDescription(void)
{
    uint8_t buffer[Encoder::kSectionHeaderBlockSize + 2 * Encoder::kInterfaceDescriptionBlockSize];
    Encoder encoder(buffer, sizeof(buffer));

    printf("TestSectionHeaderAndInterfaceDescription\n");

    SuccessOrQuit(encoder.AppendSectionHeader());
    SuccessOrQuit(encoder.AppendInterfaceDescription(kLinkTypeIeee802154Tap));
    SuccessOrQuit(encoder.AppendInterfaceDescription(kLinkTypeEthernet));
    VerifyOrQuit(encoder.GetLength() == sizeof(buffer));

    VerifyBlockTotalLength(buffer, Encoder::kSectionHeaderBlockSize);
    VerifyBlockTotalLength(buffer + Encoder::kSectionHeaderBlockSize, Encoder::kInterfaceDescriptionBlockSize);
    VerifyOrQuit(LittleEndian::ReadUint16(buffer + Encoder::kSectionHeaderBlockSize + kIdbLinkTypeOffset) ==
                 kLinkTypeIeee802154Tap);
    VerifyOrQuit(LittleEndian::ReadUint16(buffer + Encoder::kSectionHeaderBlockSize +
                                          Encoder::kInterfaceDescriptionBlockSize + kIdbLinkTypeOffset) ==
                 kLinkTypeEthernet);

    // A full buffer leaves the encoded blocks unchanged.
    VerifyOrQuit(encoder.AppendInterfaceDescription(kLinkTypeEthernet) == OT_ERROR_NO_BUFS);
    VerifyOrQuit(encoder.GetLength() == sizeof(buffer));
}

void TestIeee802154Frame(void)
{
    uint8_t      buffer[Encoder::kMaxIeee802154FrameBlockSize];
    uint8_t      psdu[sizeof(kPsdu)];
    otRadioFrame frame;
    uint32_t     length;
    float        rssi;
    uint32_t     rssiBits;

    printf("TestIeee802154Frame\n");

    memcpy(psdu, kPsdu, sizeof(psdu));
    memset(&frame, 0, sizeof(frame));
    frame.mPsdu    = psdu;
    frame.mLength  = sizeof(psdu);
    frame.mChannel = 11;

    // Without RSSI and LQI, keeping the FCS as is.
    {
        Encoder encoder(buffer, sizeof(buffer));

        SuccessOrQuit(encoder.AppendIeee802154Frame(0, 0, frame, OT_RADIO_RSSI_INVALID, OT_RADIO_LQI_NONE, kFcsAsIs));
        length = encoder.GetLength();
        VerifyOrQuit(length == GetEnhancedPacketBlockSize(kTapSizeWithoutRssiLqi + sizeof(psdu)));
        VerifyBlockTotalLength(buffer, length);
        VerifyOrQuit(LittleEndian::ReadUint16(buffer + kEpbPacketOffset + kTapLengthOffset) == kTapSizeWithoutRssiLqi);
        VerifyOrQuit(memcmp(buffer + kEpbPacketOffset + kTapSizeWithoutRssiLqi, kPsdu, sizeof(kPsdu)) == 0);
    }

    // With RSSI and LQI, recomputing the FCS.
    {
        Encoder        encoder(buffer, sizeof(buffer));
        const uint8_t *tap = buffer + kEpbPacketOffset;

        SuccessOrQuit(encoder.AppendIeee802154Frame(0, 0, frame, kRssi, kLqi, kFcsRecompute));
        length = encoder.GetLength();
        VerifyOrQuit(length == GetEnhancedPacketBlockSize(kTapSizeWithRssiLqi + sizeof(psdu)));
        VerifyBlockTotalLength(buffer, length);
        VerifyOrQuit(LittleEndian::ReadUint16(tap + kTapLengthOffset) == kTapSizeWithRssiLqi);

        VerifyOrQuit(LittleEndian::ReadUint16(tap + kTapRssiTlvOffset) == kTapRssiTlvType);
        rssiBits = LittleEndian::ReadUint32(tap + kTapRssiTlvOffset + kTapTlvValueOffset);
        memcpy(&rssi, &rssiBits, sizeof(rssi));
        VerifyOrQuit(rssi == kRssi);
        VerifyOrQuit(LittleEndian::ReadUint16(tap + kTapLqiTlvOffset) == kTapLqiTlvType);
        VerifyOrQuit(tap[kTapLqiTlvOffset + kTapTlvValueOffset] == kLqi);

        VerifyOrQuit(LittleEndian::ReadUint16(tap + kTapSizeWithRssiLqi + sizeof(psdu) - sizeof(uint16_t)) == kPsduFcs);

        // The frame itself is not modified.
        VerifyOrQuit(memcmp(psdu, kPsdu, sizeof(kPsdu)) == 0);
    }

    // Insufficient buffer space, and invalid frames.
    {
        Encoder encoder(buffer, GetEnhancedPacketBlockSize(kTapSizeWithoutRssiLqi + sizeof(psdu)) - 1);

        VerifyOrQuit(encoder.AppendIeee802154Frame(0, 0, frame, OT_RADIO_RSSI_INVALID, OT_RADIO_LQI_NONE, kFcsAsIs) ==
                     OT_ERROR_NO_BUFS);
        VerifyOrQuit(encoder.GetLength() == 0);

        frame.mLength = OT_RADIO_FRAME_MAX_SIZE + 1;
        VerifyOrQuit(encoder.AppendIeee802154Frame(0, 0, frame, OT_RADIO_RSSI_INVALID, OT_RADIO_LQI_NONE, kFcsAsIs) ==
                     OT_ERROR_INVALID_ARGS);
        VerifyOrQuit(encoder.GetLength() == 0);
    }
}

void TestEthernetFrame(void)
{
    static constexpr uint32_t kInterfaceId = 1;
    static constexpr uint64_t kTimeUs      = 0x0123456789abcdefull;
    static const uint8_t      kPayload[]   = {0x60, 0x00, 0x00, 0x00, 0x00};

    uint8_t        buffer[GetEnhancedPacketBlockSize(Encoder::kEthernetHeaderSize + sizeof(kPayload))];
    Encoder        encoder(buffer, sizeof(buffer));
    EthernetHeader header;

    printf("TestEthernetFrame\n");

    memset(header.mDestination, 0xff, sizeof(header.mDestination));
    memset(header.mSource, 0x02, sizeof(header.mSource));
    header.mEtherType = kEtherTypeIp6;

    SuccessOrQuit(encoder.AppendEthernetFrame(kInterfaceId, kTimeUs, header, kPayload, sizeof(kPayload)));
    VerifyOrQuit(encoder.GetLength() == sizeof(buffer));
    VerifyBlockTotalLength(buffer, sizeof(buffer));
    VerifyOrQuit(LittleEndian::ReadUint32(buffer + kEpbInterfaceIdOffset) == kInterfaceId);
    VerifyOrQuit(LittleEndian::ReadUint32(buffer + kEpbTimestampHighOffset) == static_cast<uint32_t>(kTimeUs >> 32));
    VerifyOrQuit(LittleEndian::ReadUint32(buffer + kEpbTimestampLowOffset) ==
                 static_cast<uint32_t>(kTimeUs & 0xffffffff));
    VerifyOrQuit(LittleEndian::ReadUint32(buffer + kEpbCapturedLengthOffset) ==
                 Encoder::kEthernetHeaderSize + sizeof(kPayload));
    VerifyOrQuit(BigEndian::ReadUint16(buffer + kEpbPacketOffset + kEthernetEtherTypeOffset) == kEtherTypeIp6);
    VerifyOrQuit(memcmp(buffer + kEpbPacketOffset + Encoder::kEthernetHeaderSize, kPayload, sizeof(kPayload)) == 0);

    VerifyOrQuit(encoder.AppendEthernetFrame(kInterfaceId, kTimeUs, header, kPayload, sizeof(kPayload)) ==
                 OT_ERROR_NO_BUFS);
    VerifyOrQuit(encoder.AppendEthernetFrame(kInterfaceId, kTimeUs, header, nullptr, sizeof(kPayload)) ==
                 OT_ERROR_INVALID_ARGS);
    VerifyOrQuit(encoder.AppendEthernetFrame(kInterfaceId, kTimeUs, header, kPayload,
                                             Encoder::kSnapLength - Encoder::kEthernetHeaderSize + 1) ==
                 OT_ERROR_INVALID_ARGS);
    VerifyOrQuit(encoder.GetLength() == sizeof(buffer));
}

} // namespace Pcapng
} // namespace ot

int main(void)
{
    ot::Pcapng::TestSectionHeaderAndInterfaceDescription();
    ot::Pcapng::TestIeee802154Frame();
    ot::Pcapng::TestEthernetFrame();

    printf("All tests passed\n");
    return 0;
}
