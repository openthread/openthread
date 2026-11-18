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

/**
 * @file
 *   This file implements a PCAPNG block encoder.
 */

#include "pcapng.hpp"

#include <string.h>

#include "common/code_utils.hpp"
#include "common/encoding.hpp"

namespace ot {
namespace Pcapng {

namespace {

constexpr uint32_t kBlockTypeSectionHeader        = 0x0a0d0d0a;
constexpr uint32_t kBlockTypeInterfaceDescription = 0x00000001;
constexpr uint32_t kBlockTypeEnhancedPacket       = 0x00000006;

constexpr uint32_t kByteOrderMagic           = 0x1a2b3c4d;
constexpr uint16_t kVersionMajor             = 1;
constexpr uint16_t kVersionMinor             = 0;
constexpr uint64_t kSectionLengthUnspecified = 0xffffffffffffffffull;

// IEEE 802.15.4 TAP header and TLVs, see https://github.com/jkcko/ieee802.15.4-tap. Every TLV value used here fits
// in 32 bits, so each TLV takes 8 bytes: type, length, and the value padded to 32 bits.
constexpr uint8_t  kTapVersion          = 0;
constexpr uint16_t kTapHeaderSize       = 4;
constexpr uint16_t kTapTlvSize          = 8;
constexpr uint16_t kTapTlvFcsType       = 0;
constexpr uint16_t kTapTlvFcsLength     = 1;
constexpr uint16_t kTapTlvRssiType      = 1;
constexpr uint16_t kTapTlvRssiLength    = 4;
constexpr uint16_t kTapTlvChannelType   = 3;
constexpr uint16_t kTapTlvChannelLength = 3;
constexpr uint16_t kTapTlvLqiType       = 10;
constexpr uint16_t kTapTlvLqiLength     = 1;
constexpr uint8_t  kTapFcsType16Bit     = 1;
constexpr uint8_t  kTapChannelPage      = 0;

constexpr uint16_t kFcsSize = sizeof(uint16_t);

static_assert(Encoder::kMaxIeee802154TapHeaderSize == kTapHeaderSize + 4 * kTapTlvSize,
              "kMaxIeee802154TapHeaderSize does not match the TAP TLVs");
static_assert(Encoder::kEthernetHeaderSize == sizeof(EthernetHeader::mDestination) + sizeof(EthernetHeader::mSource) +
                                                  sizeof(EthernetHeader::mEtherType),
              "kEthernetHeaderSize does not match EthernetHeader");

void Put8(uint8_t *&aCursor, uint8_t aValue) { *aCursor++ = aValue; }

void Put16(uint8_t *&aCursor, uint16_t aValue)
{
    LittleEndian::WriteUint16(aValue, aCursor);
    aCursor += sizeof(aValue);
}

void Put32(uint8_t *&aCursor, uint32_t aValue)
{
    LittleEndian::WriteUint32(aValue, aCursor);
    aCursor += sizeof(aValue);
}

void Put64(uint8_t *&aCursor, uint64_t aValue)
{
    LittleEndian::WriteUint64(aValue, aCursor);
    aCursor += sizeof(aValue);
}

void PutBytes(uint8_t *&aCursor, const void *aData, uint32_t aLength)
{
    memcpy(aCursor, aData, aLength);
    aCursor += aLength;
}

/**
 * Puts the type and length of a TAP TLV, and zeroes its padded value.
 *
 * @returns A pointer to the TLV value.
 */
uint8_t *PutTapTlv(uint8_t *&aCursor, uint16_t aType, uint16_t aLength)
{
    uint8_t *value;

    Put16(aCursor, aType);
    Put16(aCursor, aLength);
    value = aCursor;
    Put32(aCursor, 0);

    return value;
}

/**
 * Computes the IEEE 802.15.4 FCS (ITU-T CRC-16, reflected) of @p aLength bytes at @p aData.
 */
uint16_t ComputeFcs(const uint8_t *aData, uint16_t aLength)
{
    static constexpr uint16_t kFcsPolynomial = 0x8408; // Reflected x^16 + x^12 + x^5 + 1

    uint16_t fcs = 0;

    for (uint16_t i = 0; i < aLength; i++)
    {
        fcs ^= aData[i];

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            fcs = (fcs & 1) ? static_cast<uint16_t>((fcs >> 1) ^ kFcsPolynomial) : static_cast<uint16_t>(fcs >> 1);
        }
    }

    return fcs;
}

} // namespace

Encoder::Encoder(uint8_t *aBuffer, uint32_t aBufferSize)
    : mBuffer(aBuffer)
    , mBufferSize(aBufferSize)
    , mLength(0)
{
}

otError Encoder::AppendSectionHeader(void)
{
    otError  error  = OT_ERROR_NONE;
    uint8_t *cursor = mBuffer + mLength;

    VerifyOrExit(mBufferSize - mLength >= kSectionHeaderBlockSize, error = OT_ERROR_NO_BUFS);

    Put32(cursor, kBlockTypeSectionHeader);
    Put32(cursor, 0); // Block total length, written by `EndBlock()`
    Put32(cursor, kByteOrderMagic);
    Put16(cursor, kVersionMajor);
    Put16(cursor, kVersionMinor);
    Put64(cursor, kSectionLengthUnspecified);
    EndBlock(cursor);

exit:
    return error;
}

otError Encoder::AppendInterfaceDescription(LinkType aLinkType)
{
    otError  error  = OT_ERROR_NONE;
    uint8_t *cursor = mBuffer + mLength;

    VerifyOrExit(mBufferSize - mLength >= kInterfaceDescriptionBlockSize, error = OT_ERROR_NO_BUFS);

    Put32(cursor, kBlockTypeInterfaceDescription);
    Put32(cursor, 0); // Block total length, written by `EndBlock()`
    Put16(cursor, aLinkType);
    Put16(cursor, 0); // Reserved
    Put32(cursor, kSnapLength);
    EndBlock(cursor);

exit:
    return error;
}

otError Encoder::AppendIeee802154Frame(uint32_t            aInterfaceId,
                                       uint64_t            aTimeUs,
                                       const otRadioFrame &aFrame,
                                       int8_t              aRssi,
                                       uint8_t             aLqi,
                                       FcsMode             aFcsMode)
{
    otError  error         = OT_ERROR_NONE;
    uint16_t tapHeaderSize = kTapHeaderSize + 2 * kTapTlvSize;
    uint8_t *cursor;
    uint8_t *value;

    VerifyOrExit((aFrame.mPsdu != nullptr || aFrame.mLength == 0) && aFrame.mLength <= OT_RADIO_FRAME_MAX_SIZE,
                 error = OT_ERROR_INVALID_ARGS);

    if (aRssi != OT_RADIO_RSSI_INVALID)
    {
        tapHeaderSize += kTapTlvSize;
    }

    if (aLqi != OT_RADIO_LQI_NONE)
    {
        tapHeaderSize += kTapTlvSize;
    }

    cursor = BeginEnhancedPacket(aInterfaceId, aTimeUs, tapHeaderSize + aFrame.mLength);
    VerifyOrExit(cursor != nullptr, error = OT_ERROR_NO_BUFS);

    Put8(cursor, kTapVersion);
    Put8(cursor, 0); // Reserved
    Put16(cursor, tapHeaderSize);

    value    = PutTapTlv(cursor, kTapTlvFcsType, kTapTlvFcsLength);
    value[0] = kTapFcsType16Bit;

    if (aRssi != OT_RADIO_RSSI_INVALID)
    {
        float    rssi = static_cast<float>(aRssi);
        uint32_t rssiBits;

        static_assert(sizeof(rssi) == sizeof(rssiBits), "Size mismatch between float and uint32_t");

        memcpy(&rssiBits, &rssi, sizeof(rssiBits));
        value = PutTapTlv(cursor, kTapTlvRssiType, kTapTlvRssiLength);
        LittleEndian::WriteUint32(rssiBits, value);
    }

    value = PutTapTlv(cursor, kTapTlvChannelType, kTapTlvChannelLength);
    LittleEndian::WriteUint16(aFrame.mChannel, value);
    value[sizeof(uint16_t)] = kTapChannelPage;

    if (aLqi != OT_RADIO_LQI_NONE)
    {
        value    = PutTapTlv(cursor, kTapTlvLqiType, kTapTlvLqiLength);
        value[0] = aLqi;
    }

    if (aFrame.mLength > 0)
    {
        memcpy(cursor, aFrame.mPsdu, aFrame.mLength);

        if (aFcsMode == kFcsRecompute && aFrame.mLength >= kFcsSize)
        {
            uint16_t fcsOffset = aFrame.mLength - kFcsSize;

            LittleEndian::WriteUint16(ComputeFcs(cursor, fcsOffset), cursor + fcsOffset);
        }

        cursor += aFrame.mLength;
    }

    EndBlock(cursor);

exit:
    return error;
}

otError Encoder::AppendEthernetFrame(uint32_t              aInterfaceId,
                                     uint64_t              aTimeUs,
                                     const EthernetHeader &aHeader,
                                     const uint8_t        *aPayload,
                                     uint16_t              aPayloadLength)
{
    otError  error        = OT_ERROR_NONE;
    uint32_t packetLength = kEthernetHeaderSize + static_cast<uint32_t>(aPayloadLength);
    uint8_t *cursor;

    VerifyOrExit(aPayload != nullptr && packetLength <= kSnapLength, error = OT_ERROR_INVALID_ARGS);

    cursor = BeginEnhancedPacket(aInterfaceId, aTimeUs, packetLength);
    VerifyOrExit(cursor != nullptr, error = OT_ERROR_NO_BUFS);

    PutBytes(cursor, aHeader.mDestination, sizeof(aHeader.mDestination));
    PutBytes(cursor, aHeader.mSource, sizeof(aHeader.mSource));
    BigEndian::WriteUint16(aHeader.mEtherType, cursor);
    cursor += sizeof(aHeader.mEtherType);
    PutBytes(cursor, aPayload, aPayloadLength);
    EndBlock(cursor);

exit:
    return error;
}

uint8_t *Encoder::BeginEnhancedPacket(uint32_t aInterfaceId, uint64_t aTimeUs, uint32_t aPacketLength)
{
    uint8_t *cursor = nullptr;

    VerifyOrExit(mBufferSize - mLength >= GetEnhancedPacketBlockSize(aPacketLength));

    cursor = mBuffer + mLength;
    Put32(cursor, kBlockTypeEnhancedPacket);
    Put32(cursor, 0); // Block total length, written by `EndBlock()`
    Put32(cursor, aInterfaceId);
    Put32(cursor, static_cast<uint32_t>(aTimeUs >> 32));
    Put32(cursor, static_cast<uint32_t>(aTimeUs & 0xffffffff));
    Put32(cursor, aPacketLength); // Captured packet length
    Put32(cursor, aPacketLength); // Original packet length

exit:
    return cursor;
}

void Encoder::EndBlock(uint8_t *aCursor)
{
    uint8_t *block = mBuffer + mLength;
    uint32_t blockTotalLength;

    while (static_cast<uint32_t>(aCursor - block) % sizeof(uint32_t) != 0)
    {
        Put8(aCursor, 0);
    }

    blockTotalLength = static_cast<uint32_t>(aCursor - block) + sizeof(uint32_t);
    LittleEndian::WriteUint32(blockTotalLength, block + sizeof(uint32_t));
    Put32(aCursor, blockTotalLength);
    mLength += blockTotalLength;
}

} // namespace Pcapng
} // namespace ot
