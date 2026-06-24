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

#include "pcapng.hpp"

#include <string.h>

#include "common/code_utils.hpp"
#include "common/encoding.hpp"

namespace ot {
namespace Extcap {

template <typename Type> void ClearObject(Type &aObject) { memset(&aObject, 0, sizeof(Type)); }

struct Epb
{
    uint32_t mBlockType;
    uint32_t mBlockTotalLength;
    uint32_t mInterfaceId;
    uint32_t mTimestampHigh;
    uint32_t mTimestampLow;
    uint32_t mCapturedLen;
    uint32_t mOriginalLen;
} __attribute__((packed));

PcapngWriter::PcapngWriter(void)
    : mFile(nullptr)
{
}

PcapngWriter::~PcapngWriter(void) { Close(); }

otError PcapngWriter::Open(const char *aFilename)
{
    struct Shb
    {
        uint32_t mBlockType;
        uint32_t mBlockTotalLength;
        uint32_t mByteOrderMagic;
        uint16_t mVersionMajor;
        uint16_t mVersionMinor;
        uint64_t mSectionLength;
        uint32_t mBlockTotalLength2;
    } __attribute__((packed)) shb;

    struct Idb
    {
        uint32_t mBlockType;
        uint32_t mBlockTotalLength;
        uint16_t mLinkType;
        uint16_t mReserved;
        uint32_t mSnapLen;
        uint32_t mBlockTotalLength2;
    } __attribute__((packed)) idb;

    otError error = OT_ERROR_NONE;

    Close();

    mFile = fopen(aFilename, "wb");
    VerifyOrExit(mFile != nullptr, error = OT_ERROR_FAILED);

    // Write Section Header Block (SHB)
    ClearObject(shb);
    shb.mBlockType         = LittleEndian::HostSwap(kPcapngShbType);
    shb.mBlockTotalLength  = LittleEndian::HostSwap(static_cast<uint32_t>(sizeof(shb)));
    shb.mByteOrderMagic    = LittleEndian::HostSwap(kPcapngByteOrderMagic);
    shb.mVersionMajor      = LittleEndian::HostSwap(kPcapngVersionMajor);
    shb.mVersionMinor      = LittleEndian::HostSwap(kPcapngVersionMinor);
    shb.mSectionLength     = 0xffffffffffffffffULL;
    shb.mBlockTotalLength2 = shb.mBlockTotalLength;
    VerifyOrExit(fwrite(&shb, sizeof(shb), 1, mFile) == 1, error = OT_ERROR_FAILED);

    // Write Interface Description Block (IDB)
    ClearObject(idb);
    idb.mBlockType         = LittleEndian::HostSwap(kPcapngIdbType);
    idb.mBlockTotalLength  = LittleEndian::HostSwap(static_cast<uint32_t>(sizeof(idb)));
    idb.mLinkType          = LittleEndian::HostSwap(static_cast<uint16_t>(kPcapngLinkTypeIeee802154));
    idb.mSnapLen           = LittleEndian::HostSwap(kPcapngSnapLen);
    idb.mBlockTotalLength2 = idb.mBlockTotalLength;
    VerifyOrExit(fwrite(&idb, sizeof(idb), 1, mFile) == 1, error = OT_ERROR_FAILED);

    VerifyOrExit(fflush(mFile) == 0, error = OT_ERROR_FAILED);

exit:
    if (error != OT_ERROR_NONE)
    {
        Close();
    }
    return error;
}

void PcapngWriter::Close(void)
{
    if (mFile != nullptr)
    {
        fclose(mFile);
        mFile = nullptr;
    }
}

otError PcapngWriter::WriteFrame(const otRadioFrame &aFrame, uint64_t aTimeUs)
{
    otError error = OT_ERROR_NONE;
    Epb     epb;
    struct TapHeader
    {
        uint16_t mVersion;
        uint16_t mLength;
    } __attribute__((packed)) tapHeader;

    uint32_t tapLength;
    uint32_t packetLen;
    uint32_t paddedLen;
    uint32_t blockTotalLength;
    uint32_t padding = 0;

    VerifyOrExit(mFile != nullptr, error = OT_ERROR_INVALID_STATE);

    // TAP Header + Channel TLV (8) + FCS TLV (8) + RSSI TLV (8) + LQI TLV (8)
    tapLength = sizeof(tapHeader) + 8 + 8 + 8 + 8;

    packetLen        = tapLength + aFrame.mLength;
    paddedLen        = (packetLen + 3) & ~3u;
    blockTotalLength = sizeof(epb) + paddedLen + sizeof(uint32_t);

    // Write EPB Header
    ClearObject(epb);
    epb.mBlockType        = LittleEndian::HostSwap(kPcapngEpbType);
    epb.mBlockTotalLength = LittleEndian::HostSwap(blockTotalLength);
    epb.mInterfaceId      = LittleEndian::HostSwap(0u);
    epb.mTimestampHigh    = LittleEndian::HostSwap(static_cast<uint32_t>(aTimeUs >> 32));
    epb.mTimestampLow     = LittleEndian::HostSwap(static_cast<uint32_t>(aTimeUs & 0xffffffff));
    epb.mCapturedLen      = LittleEndian::HostSwap(packetLen);
    epb.mOriginalLen      = LittleEndian::HostSwap(packetLen);
    VerifyOrExit(fwrite(&epb, sizeof(epb), 1, mFile) == 1, error = OT_ERROR_FAILED);

    // Write TAP Common Header
    ClearObject(tapHeader);
    tapHeader.mVersion = LittleEndian::HostSwap(static_cast<uint16_t>(kTapVersion));
    tapHeader.mLength  = LittleEndian::HostSwap(static_cast<uint16_t>(tapLength));
    VerifyOrExit(fwrite(&tapHeader, sizeof(tapHeader), 1, mFile) == 1, error = OT_ERROR_FAILED);

    // Write Channel TLV (8 bytes)
    uint8_t channelTlv[8];
    memset(channelTlv, 0, sizeof(channelTlv));
    LittleEndian::WriteUint16(kTapChannelType, &channelTlv[0]);
    LittleEndian::WriteUint16(kTapChannelLength, &channelTlv[2]);
    LittleEndian::WriteUint16(aFrame.mChannel, &channelTlv[4]);
    channelTlv[6] = kTapChannelPage;
    VerifyOrExit(fwrite(channelTlv, sizeof(channelTlv), 1, mFile) == 1, error = OT_ERROR_FAILED);

    // Write FCS TLV (8 bytes)
    uint8_t fcsTlv[8];
    memset(fcsTlv, 0, sizeof(fcsTlv));
    LittleEndian::WriteUint16(kTapFcsType, &fcsTlv[0]);
    LittleEndian::WriteUint16(kTapFcsLength, &fcsTlv[2]);
    fcsTlv[4] = kTapFcsValue;
    VerifyOrExit(fwrite(fcsTlv, sizeof(fcsTlv), 1, mFile) == 1, error = OT_ERROR_FAILED);

    // Write RSSI TLV (8 bytes)
    {
        uint8_t rssiTlv[8];
        memset(rssiTlv, 0, sizeof(rssiTlv));
        LittleEndian::WriteUint16(kTapRssiType, &rssiTlv[0]);
        LittleEndian::WriteUint16(kTapRssiLength, &rssiTlv[2]);
        float rssiVal = static_cast<float>(aFrame.mInfo.mRxInfo.mRssi);
        uint32_t rssiValUint;
        memcpy(&rssiValUint, &rssiVal, sizeof(uint32_t));
        rssiValUint = LittleEndian::HostSwap(rssiValUint);
        memcpy(&rssiTlv[4], &rssiValUint, sizeof(uint32_t));
        VerifyOrExit(fwrite(rssiTlv, sizeof(rssiTlv), 1, mFile) == 1, error = OT_ERROR_FAILED);
    }

    // Write LQI TLV (8 bytes)
    uint8_t lqiTlv[8];
    memset(lqiTlv, 0, sizeof(lqiTlv));
    LittleEndian::WriteUint16(kTapLqiType, &lqiTlv[0]);
    LittleEndian::WriteUint16(kTapLqiLength, &lqiTlv[2]);
    lqiTlv[4] = aFrame.mInfo.mRxInfo.mLqi;
    VerifyOrExit(fwrite(lqiTlv, sizeof(lqiTlv), 1, mFile) == 1, error = OT_ERROR_FAILED);

    // Write PSDU (Payload)
    VerifyOrExit(fwrite(aFrame.mPsdu, aFrame.mLength, 1, mFile) == 1, error = OT_ERROR_FAILED);

    // Write Padding
    if (paddedLen > packetLen)
    {
        VerifyOrExit(fwrite(&padding, paddedLen - packetLen, 1, mFile) == 1, error = OT_ERROR_FAILED);
    }

    // Write EPB Footer (re-write BlockTotalLength)
    VerifyOrExit(fwrite(&epb.mBlockTotalLength, sizeof(epb.mBlockTotalLength), 1, mFile) == 1, error = OT_ERROR_FAILED);

    VerifyOrExit(fflush(mFile) == 0, error = OT_ERROR_FAILED);

exit:
    if (error != OT_ERROR_NONE)
    {
        Close();
    }
    return error;
}

} // namespace Extcap
} // namespace ot
