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

#include "nexus_pcap.hpp"
#include "common/clearable.hpp"
#include "common/code_utils.hpp"
#include "common/encoding.hpp"

namespace ot {
namespace Nexus {

Pcap::Pcap(void)
    : mFile(nullptr)
{
}

Pcap::~Pcap(void) { Close(); }

void Pcap::Open(const char *aFilename)
{
    OT_TOOL_PACKED_BEGIN
    struct Shb
    {
        uint32_t mBlockType;
        uint32_t mBlockTotalLength;
        uint32_t mByteOrderMagic;
        uint16_t mVersionMajor;
        uint16_t mVersionMinor;
        uint64_t mSectionLength;
        uint32_t mBlockTotalLength2;
    } OT_TOOL_PACKED_END shb;

    OT_TOOL_PACKED_BEGIN
    struct Idb
    {
        uint32_t mBlockType;
        uint32_t mBlockTotalLength;
        uint16_t mLinkType;
        uint16_t mReserved;
        uint32_t mSnapLen;
        uint32_t mBlockTotalLength2;
    } OT_TOOL_PACKED_END idb;

    Close();

    mFile = fopen(aFilename, "wb");
    VerifyOrExit(mFile != nullptr);

    ClearAllBytes(shb);
    shb.mBlockType         = LittleEndian::HostSwap(kPcapngShbType);
    shb.mBlockTotalLength  = LittleEndian::HostSwap(static_cast<uint32_t>(sizeof(shb)));
    shb.mByteOrderMagic    = LittleEndian::HostSwap(kPcapngByteOrderMagic);
    shb.mVersionMajor      = LittleEndian::HostSwap(kPcapngVersionMajor);
    shb.mVersionMinor      = LittleEndian::HostSwap(kPcapngVersionMinor);
    shb.mSectionLength     = 0xffffffffffffffffULL;
    shb.mBlockTotalLength2 = shb.mBlockTotalLength;
    VerifyOrExit(fwrite(&shb, sizeof(shb), 1, mFile) == 1, Close());

    ClearAllBytes(idb);
    idb.mBlockType         = LittleEndian::HostSwap(kPcapngIdbType);
    idb.mBlockTotalLength  = LittleEndian::HostSwap(static_cast<uint32_t>(sizeof(idb)));
    idb.mLinkType          = LittleEndian::HostSwap(static_cast<uint16_t>(kPcapngLinkTypeIeee802154));
    idb.mSnapLen           = LittleEndian::HostSwap(kPcapngSnapLen);
    idb.mBlockTotalLength2 = idb.mBlockTotalLength;
    VerifyOrExit(fwrite(&idb, sizeof(idb), 1, mFile) == 1, Close());

    VerifyOrExit(fflush(mFile) == 0, Close());

exit:
    return;
}

void Pcap::Close(void)
{
    VerifyOrExit(mFile != nullptr);

    fclose(mFile);
    mFile = nullptr;

exit:
    return;
}

void Pcap::WriteFrame(const otRadioFrame &aFrame, uint64_t aTimeUs)
{
    OT_TOOL_PACKED_BEGIN
    struct Epb
    {
        uint32_t mBlockType;
        uint32_t mBlockTotalLength;
        uint32_t mInterfaceId;
        uint32_t mTimestampHigh;
        uint32_t mTimestampLow;
        uint32_t mCapturedLen;
        uint32_t mOriginalLen;
    } OT_TOOL_PACKED_END epb;

    OT_TOOL_PACKED_BEGIN
    struct TapHeader
    {
        uint16_t mVersion;
        uint16_t mLength;
    } OT_TOOL_PACKED_END tapHeader;

    uint32_t tapLength;
    uint32_t packetLen;
    uint32_t paddedLen;
    uint32_t blockTotalLength;
    uint32_t padding = 0;

    VerifyOrExit(mFile != nullptr);

    tapLength = sizeof(tapHeader) + 8 + 8;
    // Note: tshark expects specific lengths for TLVs.
    // FCS TLV: Type(2), Length(2), Value(1), Padding(3) -> Total 8 bytes. Value length = 1.
    // Channel TLV: Type(2), Length(2), Channel(2), Page(1), Padding(1) -> Total 8 bytes. Value length = 3.

    packetLen        = tapLength + aFrame.mLength;
    paddedLen        = (packetLen + 3) & ~3u;
    blockTotalLength = sizeof(epb) + paddedLen + sizeof(uint32_t);

    ClearAllBytes(epb);
    epb.mBlockType        = LittleEndian::HostSwap(kPcapngEpbType);
    epb.mBlockTotalLength = LittleEndian::HostSwap(blockTotalLength);
    epb.mInterfaceId      = 0;
    epb.mTimestampHigh    = LittleEndian::HostSwap(static_cast<uint32_t>(aTimeUs >> 32));
    epb.mTimestampLow     = LittleEndian::HostSwap(static_cast<uint32_t>(aTimeUs & 0xffffffff));
    epb.mCapturedLen      = LittleEndian::HostSwap(packetLen);
    epb.mOriginalLen      = LittleEndian::HostSwap(packetLen);
    VerifyOrExit(fwrite(&epb, sizeof(epb), 1, mFile) == 1, Close());

    ClearAllBytes(tapHeader);
    tapHeader.mVersion = LittleEndian::HostSwap(static_cast<uint16_t>(kTapVersion));
    tapHeader.mLength  = LittleEndian::HostSwap(static_cast<uint16_t>(tapLength));
    VerifyOrExit(fwrite(&tapHeader, sizeof(tapHeader), 1, mFile) == 1, Close());

    // FCS TLV
    uint8_t fcsTlv[8];
    ClearAllBytes(fcsTlv);
    LittleEndian::WriteUint16(kTapFcsType, &fcsTlv[0]);
    LittleEndian::WriteUint16(kTapFcsLength, &fcsTlv[2]);
    fcsTlv[4] = kTapFcsValue;
    VerifyOrExit(fwrite(fcsTlv, sizeof(fcsTlv), 1, mFile) == 1, Close());

    // Channel TLV
    uint8_t channelTlv[8];
    ClearAllBytes(channelTlv);
    LittleEndian::WriteUint16(kTapChannelType, &channelTlv[0]);
    LittleEndian::WriteUint16(kTapChannelLength, &channelTlv[2]);
    LittleEndian::WriteUint16(aFrame.mChannel, &channelTlv[4]);
    channelTlv[6] = kTapChannelPage;
    VerifyOrExit(fwrite(channelTlv, sizeof(channelTlv), 1, mFile) == 1, Close());

    VerifyOrExit(fwrite(aFrame.mPsdu, aFrame.mLength, 1, mFile) == 1, Close());

    if (paddedLen > packetLen)
    {
        VerifyOrExit(fwrite(&padding, paddedLen - packetLen, 1, mFile) == 1, Close());
    }

    VerifyOrExit(fwrite(&epb.mBlockTotalLength, sizeof(epb.mBlockTotalLength), 1, mFile) == 1, Close());

    VerifyOrExit(fflush(mFile) == 0, Close());

exit:
    return;
}

} // namespace Nexus
} // namespace ot
