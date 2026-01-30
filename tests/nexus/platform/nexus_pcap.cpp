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
    struct PcapHeader
    {
        uint32_t mMagicNumber;
        uint16_t mVersionMajor;
        uint16_t mVersionMinor;
        int32_t  mThisZone;
        uint32_t mSigFigs;
        uint32_t mSnapLen;
        uint32_t mNetwork;
    } OT_TOOL_PACKED_END header;

    Close();

    mFile = fopen(aFilename, "wb");
    VerifyOrExit(mFile != nullptr);

    ClearAllBytes(header);
    header.mMagicNumber  = LittleEndian::HostSwap(kPcapMagicNumber);
    header.mVersionMajor = LittleEndian::HostSwap(kPcapVersionMajor);
    header.mVersionMinor = LittleEndian::HostSwap(kPcapVersionMinor);
    header.mSnapLen      = LittleEndian::HostSwap(kPcapSnapLen);
    header.mNetwork      = LittleEndian::HostSwap(kPcapDlt154Tap);

    VerifyOrExit(fwrite(&header, sizeof(header), 1, mFile) == 1, Close());
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
    struct PcapRecordHeader
    {
        uint32_t mTsSec;
        uint32_t mTsUsec;
        uint32_t mInclLen;
        uint32_t mOrigLen;
    } OT_TOOL_PACKED_END recordHeader;

    OT_TOOL_PACKED_BEGIN
    struct TapHeader
    {
        uint8_t  mVersion;
        uint8_t  mReserved;
        uint16_t mLength;
    } OT_TOOL_PACKED_END tapHeader;

    OT_TOOL_PACKED_BEGIN
    struct TapFcsTlv
    {
        uint16_t mType;
        uint16_t mLength;
        uint8_t  mValue;
        uint8_t  mPadding[3];
    } OT_TOOL_PACKED_END fcsTlv;

    OT_TOOL_PACKED_BEGIN
    struct TapChannelTlv
    {
        uint16_t mType;
        uint16_t mLength;
        uint8_t  mPage;
        uint16_t mChannel;
        uint8_t  mPadding[1];
    } OT_TOOL_PACKED_END channelTlv;

    uint32_t tapLength;

    VerifyOrExit(mFile != nullptr);

    tapLength = sizeof(tapHeader) + sizeof(fcsTlv) + sizeof(channelTlv);

    ClearAllBytes(recordHeader);
    recordHeader.mTsSec   = LittleEndian::HostSwap(static_cast<uint32_t>(aTimeUs / 1000000));
    recordHeader.mTsUsec  = LittleEndian::HostSwap(static_cast<uint32_t>(aTimeUs % 1000000));
    recordHeader.mInclLen = LittleEndian::HostSwap(tapLength + aFrame.mLength);
    recordHeader.mOrigLen = recordHeader.mInclLen;
    VerifyOrExit(fwrite(&recordHeader, sizeof(recordHeader), 1, mFile) == 1, Close());

    ClearAllBytes(tapHeader);
    tapHeader.mVersion = LittleEndian::HostSwap(kTapVersion);
    tapHeader.mLength  = LittleEndian::HostSwap(static_cast<uint16_t>(tapLength));
    VerifyOrExit(fwrite(&tapHeader, sizeof(tapHeader), 1, mFile) == 1, Close());

    ClearAllBytes(fcsTlv);
    fcsTlv.mType   = LittleEndian::HostSwap(kTapFcsType);
    fcsTlv.mLength = LittleEndian::HostSwap(kTapFcsLength);
    fcsTlv.mValue  = LittleEndian::HostSwap(kTapFcsValue);
    VerifyOrExit(fwrite(&fcsTlv, sizeof(fcsTlv), 1, mFile) == 1, Close());

    ClearAllBytes(channelTlv);
    channelTlv.mType    = LittleEndian::HostSwap(kTapChannelType);
    channelTlv.mLength  = LittleEndian::HostSwap(kTapChannelLength);
    channelTlv.mPage    = LittleEndian::HostSwap(kTapChannelPage);
    channelTlv.mChannel = LittleEndian::HostSwap(aFrame.mChannel);
    VerifyOrExit(fwrite(&channelTlv, sizeof(channelTlv), 1, mFile) == 1, Close());

    VerifyOrExit(fwrite(aFrame.mPsdu, aFrame.mLength, 1, mFile) == 1, Close());
    VerifyOrExit(fflush(mFile) == 0, Close());

exit:
    return;
}

} // namespace Nexus
} // namespace ot
