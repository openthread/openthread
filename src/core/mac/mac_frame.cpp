/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file implements IEEE 802.15.4 header generation and processing.
 */

#include "mac_frame.hpp"

#include <stdio.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/frame_builder.hpp"
#include "common/log.hpp"
#include "radio/trel_link.hpp"
#if !OPENTHREAD_RADIO || OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE
#include "crypto/aes_ccm.hpp"
#endif

namespace ot {
namespace Mac {

void HeaderIe::Init(uint16_t aId, uint8_t aLen)
{
    Init();
    SetId(aId);
    SetLength(aLen);
}

void Frame::InitMacHeader(Type             aType,
                          Version          aVersion,
                          const Addresses &aAddrs,
                          const PanIds    &aPanIds,
                          SecurityLevel    aSecurityLevel,
                          KeyIdMode        aKeyIdMode)
{
    uint16_t     fcf;
    FrameBuilder builder;

    fcf = static_cast<uint16_t>(aType) | static_cast<uint16_t>(aVersion);

    switch (aAddrs.mSource.GetType())
    {
    case Address::kTypeNone:
        fcf |= kFcfSrcAddrNone;
        break;
    case Address::kTypeShort:
        fcf |= kFcfSrcAddrShort;
        break;
    case Address::kTypeExtended:
        fcf |= kFcfSrcAddrExt;
        break;
    }

    switch (aAddrs.mDestination.GetType())
    {
    case Address::kTypeNone:
        fcf |= kFcfDstAddrNone;
        break;
    case Address::kTypeShort:
        fcf |= kFcfDstAddrShort;
        fcf |= ((aAddrs.mDestination.GetShort() == kShortAddrBroadcast) ? 0 : kFcfAckRequest);
        break;
    case Address::kTypeExtended:
        fcf |= (kFcfDstAddrExt | kFcfAckRequest);
        break;
    }

    if (aType == kTypeAck)
    {
        fcf &= ~kFcfAckRequest;
    }

    fcf |= (aSecurityLevel != kSecurityNone) ? kFcfSecurityEnabled : 0;

    // PAN ID compression

    switch (aVersion)
    {
    case kVersion2003:
    case kVersion2006:

        // For 2003-2006 versions:
        //
        // - If only either the destination or the source addressing information is present,
        //   the PAN ID Compression field shall be set to zero, and the PAN ID field of the
        //   single address shall be included in the transmitted frame.
        // - If both destination and source addressing information is present, the MAC shall
        //   compare the destination and source PAN identifiers. If the PAN IDs are identical,
        //   the PAN ID Compression field shall be set to one, and the Source PAN ID field
        //   shall be omitted from the transmitted frame. If the PAN IDs are different, the
        //   PAN ID Compression field shall be set to zero, and both Destination PAN ID
        //   field and Source PAN ID fields shall be included in the transmitted frame.

        if (!aAddrs.mSource.IsNone() && !aAddrs.mDestination.IsNone() &&
            (aPanIds.GetSource() == aPanIds.GetDestination()))
        {
            fcf |= kFcfPanidCompression;
        }
        break;

    case kVersion2015:
        // +----+--------------+--------------+--------------+--------------+--------------+
        // | No |  Dest Addr   |   Src Addr   |   Dst PAN ID |  Src PAN ID  |  PAN ID Comp |
        // +----+--------------+--------------+--------------+--------------+--------------+
        // |  1 | Not Present  | Not Present  | Not Present  | Not Present  |      0       |
        // |  2 | Not Present  | Not Present  | Present      | Not Present  |      1       |
        // |  3 | Present      | Not Present  | Present      | Not Present  |      0       |
        // |  4 | Present      | Not Present  | Not Present  | Not Present  |      1       |
        // |  5 | Not Present  | Present      | Not Present  | Present      |      0       |
        // |  6 | Not Present  | Present      | Not Present  | Not Present  |      1       |
        // +----+--------------+--------------+--------------+--------------+--------------+
        // |  7 | Extended     | Extended     | Present      | Not Present  |      0       |
        // |  8 | Extended     | Extended     | Not Present  | Not Present  |      1       |
        // |----+--------------+--------------+--------------+--------------+--------------+
        // |  9 | Short        | Short        | Present      | Present      |      0       |
        // | 10 | Short        | Extended     | Present      | Present      |      0       |
        // | 11 | Extended     | Short        | Present      | Present      |      0       |
        // | 12 | Short        | Extended     | Present      | Not Present  |      1       |
        // | 13 | Extended     | Short        | Present      | Not Present  |      1       |
        // | 14 | Short        | Short        | Present      | Not Present  |      1       |
        // +----+--------------+--------------+--------------+--------------+--------------+

        if (aAddrs.mDestination.IsNone())
        {
            // Dst addr not present - rows 1,2,5,6.

            if ((aAddrs.mSource.IsNone() && aPanIds.IsDestinationPresent()) ||                               // Row 2.
                (!aAddrs.mSource.IsNone() && !aPanIds.IsDestinationPresent() && !aPanIds.IsSourcePresent())) // Row 6.
            {
                fcf |= kFcfPanidCompression;
            }

            break;
        }

        if (aAddrs.mSource.IsNone())
        {
            // Dst addr present, Src addr not present - rows 3,4.

            if (!aPanIds.IsDestinationPresent()) // Row 4.
            {
                fcf |= kFcfPanidCompression;
            }

            break;
        }

        // Both addresses are present - rows 7 to 14.

        if (aAddrs.mSource.IsExtended() && aAddrs.mDestination.IsExtended())
        {
            // Both addresses are extended - rows 7,8.

            if (aPanIds.IsDestinationPresent()) // Row 7.
            {
                break;
            }
        }
        else if (aPanIds.GetSource() != aPanIds.GetDestination()) // Rows 9-14.
        {
            break;
        }

        fcf |= kFcfPanidCompression;

        break;
    }

    builder.Init(mPsdu, GetMtu());
    IgnoreError(builder.AppendLittleEndianUint16(fcf));
    IgnoreError(builder.AppendUint8(0)); // Seq number

    if (IsDstPanIdPresent(fcf))
    {
        IgnoreError(builder.AppendLittleEndianUint16(aPanIds.GetDestination()));
    }

    IgnoreError(builder.AppendMacAddress(aAddrs.mDestination));

    if (IsSrcPanIdPresent(fcf))
    {
        IgnoreError(builder.AppendLittleEndianUint16(aPanIds.GetSource()));
    }

    IgnoreError(builder.AppendMacAddress(aAddrs.mSource));

    mLength = builder.GetLength();

    if (aSecurityLevel != kSecurityNone)
    {
        uint8_t secCtl = static_cast<uint8_t>(aSecurityLevel) | static_cast<uint8_t>(aKeyIdMode);

        IgnoreError(builder.AppendUint8(secCtl));

        mLength += CalculateSecurityHeaderSize(secCtl);
        mLength += CalculateMicSize(secCtl);
    }

    if (aType == kTypeMacCmd)
    {
        mLength += kCommandIdSize;
    }

    mLength += GetFcsSize();
}

uint16_t Frame::GetFrameControlField(void) const { return LittleEndian::ReadUint16(mPsdu); }

void Frame::SetFrameControlField(uint16_t aFcf) { LittleEndian::WriteUint16(aFcf, mPsdu); }

Error Frame::ValidatePsdu(void) const
{
    Error   error = kErrorNone;
    uint8_t index = FindPayloadIndex();

    VerifyOrExit(index != kInvalidIndex, error = kErrorParse);
    VerifyOrExit((index + GetFooterLength()) <= mLength, error = kErrorParse);

exit:
    return error;
}

void Frame::SetAckRequest(bool aAckRequest)
{
    if (aAckRequest)
    {
        mPsdu[0] |= kFcfAckRequest;
    }
    else
    {
        mPsdu[0] &= ~kFcfAckRequest;
    }
}

void Frame::SetFramePending(bool aFramePending)
{
    if (aFramePending)
    {
        mPsdu[0] |= kFcfFramePending;
    }
    else
    {
        mPsdu[0] &= ~kFcfFramePending;
    }
}

void Frame::SetIePresent(bool aIePresent)
{
    uint16_t fcf = GetFrameControlField();

    if (aIePresent)
    {
        fcf |= kFcfIePresent;
    }
    else
    {
        fcf &= ~kFcfIePresent;
    }

    SetFrameControlField(fcf);
}

uint8_t Frame::FindDstPanIdIndex(void) const
{
    uint8_t index;

    VerifyOrExit(IsDstPanIdPresent(), index = kInvalidIndex);

    index = kFcfSize + kDsnSize;

exit:
    return index;
}

bool Frame::IsDstPanIdPresent(uint16_t aFcf)
{
    bool present = true;

    if (IsVersion2015(aFcf))
    {
        // Original table at `InitMacHeader()`
        //
        // +----+--------------+--------------+--------------++--------------+
        // | No |  Dest Addr   |   Src Addr   |  PAN ID Comp ||   Dst PAN ID |
        // +----+--------------+--------------+--------------++--------------+
        // |  1 | Not Present  | Not Present  |      0       || Not Present  |
        // |  2 | Not Present  | Not Present  |      1       || Present      |
        // |  3 | Present      | Not Present  |      0       || Present      |
        // |  4 | Present      | Not Present  |      1       || Not Present  |
        // |  5 | Not Present  | Present      |      0       || Not Present  |
        // |  6 | Not Present  | Present      |      1       || Not Present  |
        // +----+--------------+--------------+--------------++--------------+
        // |  7 | Extended     | Extended     |      0       || Present      |
        // |  8 | Extended     | Extended     |      1       || Not Present  |
        // |----+--------------+--------------+--------------++--------------+
        // |  9 | Short        | Short        |      0       || Present      |
        // | 10 | Short        | Extended     |      0       || Present      |
        // | 11 | Extended     | Short        |      0       || Present      |
        // | 12 | Short        | Extended     |      1       || Present      |
        // | 13 | Extended     | Short        |      1       || Present      |
        // | 14 | Short        | Short        |      1       || Present      |
        // +----+--------------+--------------+--------------++--------------+

        switch (aFcf & (kFcfDstAddrMask | kFcfSrcAddrMask | kFcfPanidCompression))
        {
        case (kFcfDstAddrNone | kFcfSrcAddrNone):                         // 1
        case (kFcfDstAddrShort | kFcfSrcAddrNone | kFcfPanidCompression): // 4 (short dst)
        case (kFcfDstAddrExt | kFcfSrcAddrNone | kFcfPanidCompression):   // 4 (ext dst)
        case (kFcfDstAddrNone | kFcfSrcAddrShort):                        // 5 (short src)
        case (kFcfDstAddrNone | kFcfSrcAddrExt):                          // 5 (ext src)
        case (kFcfDstAddrNone | kFcfSrcAddrShort | kFcfPanidCompression): // 6 (short src)
        case (kFcfDstAddrNone | kFcfSrcAddrExt | kFcfPanidCompression):   // 6 (ext src)
        case (kFcfDstAddrExt | kFcfSrcAddrExt | kFcfPanidCompression):    // 8
            present = false;
            break;
        default:
            break;
        }
    }
    else
    {
        present = IsDstAddrPresent(aFcf);
    }

    return present;
}

Error Frame::GetDstPanId(PanId &aPanId) const
{
    Error   error = kErrorNone;
    uint8_t index = FindDstPanIdIndex();

    VerifyOrExit(index != kInvalidIndex, error = kErrorParse);
    aPanId = LittleEndian::ReadUint16(&mPsdu[index]);

exit:
    return error;
}

void Frame::SetDstPanId(PanId aPanId)
{
    uint8_t index = FindDstPanIdIndex();

    OT_ASSERT(index != kInvalidIndex);
    LittleEndian::WriteUint16(aPanId, &mPsdu[index]);
}

uint8_t Frame::FindDstAddrIndex(void) const { return kFcfSize + kDsnSize + (IsDstPanIdPresent() ? sizeof(PanId) : 0); }

Error Frame::GetDstAddr(Address &aAddress) const
{
    Error   error = kErrorNone;
    uint8_t index = FindDstAddrIndex();

    VerifyOrExit(index != kInvalidIndex, error = kErrorParse);

    switch (GetFrameControlField() & kFcfDstAddrMask)
    {
    case kFcfDstAddrShort:
        aAddress.SetShort(LittleEndian::ReadUint16(&mPsdu[index]));
        break;

    case kFcfDstAddrExt:
        aAddress.SetExtended(&mPsdu[index], ExtAddress::kReverseByteOrder);
        break;

    default:
        aAddress.SetNone();
        break;
    }

exit:
    return error;
}

void Frame::SetDstAddr(ShortAddress aShortAddress)
{
    OT_ASSERT((GetFrameControlField() & kFcfDstAddrMask) == kFcfDstAddrShort);
    LittleEndian::WriteUint16(aShortAddress, &mPsdu[FindDstAddrIndex()]);
}

void Frame::SetDstAddr(const ExtAddress &aExtAddress)
{
    uint8_t index = FindDstAddrIndex();

    OT_ASSERT((GetFrameControlField() & kFcfDstAddrMask) == kFcfDstAddrExt);
    OT_ASSERT(index != kInvalidIndex);

    aExtAddress.CopyTo(&mPsdu[index], ExtAddress::kReverseByteOrder);
}

void Frame::SetDstAddr(const Address &aAddress)
{
    switch (aAddress.GetType())
    {
    case Address::kTypeShort:
        SetDstAddr(aAddress.GetShort());
        break;

    case Address::kTypeExtended:
        SetDstAddr(aAddress.GetExtended());
        break;

    default:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);
    }
}

uint8_t Frame::FindSrcPanIdIndex(void) const
{
    uint8_t  index = 0;
    uint16_t fcf   = GetFrameControlField();

    VerifyOrExit(IsSrcPanIdPresent(), index = kInvalidIndex);

    index += kFcfSize + kDsnSize;

    if (IsDstPanIdPresent(fcf))
    {
        index += sizeof(PanId);
    }

    switch (fcf & kFcfDstAddrMask)
    {
    case kFcfDstAddrShort:
        index += sizeof(ShortAddress);
        break;

    case kFcfDstAddrExt:
        index += sizeof(ExtAddress);
        break;
    }

exit:
    return index;
}

bool Frame::IsSrcPanIdPresent(uint16_t aFcf)
{
    bool present = IsSrcAddrPresent(aFcf) && ((aFcf & kFcfPanidCompression) == 0);

    // Special case for a IEEE 802.15.4-2015 frame: When both
    // addresses are extended, then the source PAN iD is not present
    // independent of PAN ID Compression. In this case, if the PAN ID
    // compression is set, it indicates that no PAN ID is in the
    // frame, while if the PAN ID Compression is zero, it indicates
    // the presence of the destination PAN ID in the frame.
    //
    // +----+--------------+--------------+--------------++--------------+
    // | No |  Dest Addr   |   Src Addr   |  PAN ID Comp ||  Src PAN ID  |
    // +----+--------------+--------------+--------------++--------------+
    // |  1 | Not Present  | Not Present  |      0       || Not Present  |
    // |  2 | Not Present  | Not Present  |      1       || Not Present  |
    // |  3 | Present      | Not Present  |      0       || Not Present  |
    // |  4 | Present      | Not Present  |      1       || Not Present  |
    // |  5 | Not Present  | Present      |      0       || Present      |
    // |  6 | Not Present  | Present      |      1       || Not Present  |
    // +----+--------------+--------------+--------------++--------------+
    // |  7 | Extended     | Extended     |      0       || Not Present  |
    // |  8 | Extended     | Extended     |      1       || Not Present  |
    // |----+--------------+--------------+--------------++--------------+
    // |  9 | Short        | Short        |      0       || Present      |
    // | 10 | Short        | Extended     |      0       || Present      |
    // | 11 | Extended     | Short        |      0       || Present      |
    // | 12 | Short        | Extended     |      1       || Not Present  |
    // | 13 | Extended     | Short        |      1       || Not Present  |
    // | 14 | Short        | Short        |      1       || Not Present  |
    // +----+--------------+--------------+--------------++--------------+

    if (IsVersion2015(aFcf) && ((aFcf & (kFcfDstAddrMask | kFcfSrcAddrMask)) == (kFcfDstAddrExt | kFcfSrcAddrExt)))
    {
        present = false;
    }

    return present;
}

Error Frame::GetSrcPanId(PanId &aPanId) const
{
    Error   error = kErrorNone;
    uint8_t index = FindSrcPanIdIndex();

    VerifyOrExit(index != kInvalidIndex, error = kErrorParse);
    aPanId = LittleEndian::ReadUint16(&mPsdu[index]);

exit:
    return error;
}

Error Frame::SetSrcPanId(PanId aPanId)
{
    Error   error = kErrorNone;
    uint8_t index = FindSrcPanIdIndex();

    VerifyOrExit(index != kInvalidIndex, error = kErrorParse);
    LittleEndian::WriteUint16(aPanId, &mPsdu[index]);

exit:
    return error;
}

uint8_t Frame::FindSrcAddrIndex(void) const
{
    uint8_t  index = 0;
    uint16_t fcf   = GetFrameControlField();

    index += kFcfSize + kDsnSize;

    if (IsDstPanIdPresent(fcf))
    {
        index += sizeof(PanId);
    }

    switch (fcf & kFcfDstAddrMask)
    {
    case kFcfDstAddrShort:
        index += sizeof(ShortAddress);
        break;

    case kFcfDstAddrExt:
        index += sizeof(ExtAddress);
        break;
    }

    if (IsSrcPanIdPresent(fcf))
    {
        index += sizeof(PanId);
    }

    return index;
}

Error Frame::GetSrcAddr(Address &aAddress) const
{
    Error    error = kErrorNone;
    uint8_t  index = FindSrcAddrIndex();
    uint16_t fcf   = GetFrameControlField();

    VerifyOrExit(index != kInvalidIndex, error = kErrorParse);

    switch (fcf & kFcfSrcAddrMask)
    {
    case kFcfSrcAddrShort:
        aAddress.SetShort(LittleEndian::ReadUint16(&mPsdu[index]));
        break;

    case kFcfSrcAddrExt:
        aAddress.SetExtended(&mPsdu[index], ExtAddress::kReverseByteOrder);
        break;

    case kFcfSrcAddrNone:
        aAddress.SetNone();
        break;

    default:
        // reserved value
        error = kErrorParse;
        break;
    }

exit:
    return error;
}

void Frame::SetSrcAddr(ShortAddress aShortAddress)
{
    uint8_t index = FindSrcAddrIndex();

    OT_ASSERT((GetFrameControlField() & kFcfSrcAddrMask) == kFcfSrcAddrShort);
    OT_ASSERT(index != kInvalidIndex);

    LittleEndian::WriteUint16(aShortAddress, &mPsdu[index]);
}

void Frame::SetSrcAddr(const ExtAddress &aExtAddress)
{
    uint8_t index = FindSrcAddrIndex();

    OT_ASSERT((GetFrameControlField() & kFcfSrcAddrMask) == kFcfSrcAddrExt);
    OT_ASSERT(index != kInvalidIndex);

    aExtAddress.CopyTo(&mPsdu[index], ExtAddress::kReverseByteOrder);
}

void Frame::SetSrcAddr(const Address &aAddress)
{
    switch (aAddress.GetType())
    {
    case Address::kTypeShort:
        SetSrcAddr(aAddress.GetShort());
        break;

    case Address::kTypeExtended:
        SetSrcAddr(aAddress.GetExtended());
        break;

    default:
        OT_ASSERT(false);
    }
}

Error Frame::GetSecurityControlField(uint8_t &aSecurityControlField) const
{
    Error   error = kErrorNone;
    uint8_t index = FindSecurityHeaderIndex();

    VerifyOrExit(index != kInvalidIndex, error = kErrorParse);

    aSecurityControlField = mPsdu[index];

exit:
    return error;
}

void Frame::SetSecurityControlField(uint8_t aSecurityControlField)
{
    uint8_t index = FindSecurityHeaderIndex();

    OT_ASSERT(index != kInvalidIndex);

    mPsdu[index] = aSecurityControlField;
}

uint8_t Frame::FindSecurityHeaderIndex(void) const
{
    uint8_t index;

    VerifyOrExit(kFcfSize < mLength, index = kInvalidIndex);
    VerifyOrExit(GetSecurityEnabled(), index = kInvalidIndex);
    index = SkipAddrFieldIndex();

exit:
    return index;
}

Error Frame::GetSecurityLevel(uint8_t &aSecurityLevel) const
{
    Error   error = kErrorNone;
    uint8_t index = FindSecurityHeaderIndex();

    VerifyOrExit(index != kInvalidIndex, error = kErrorParse);

    aSecurityLevel = mPsdu[index] & kSecLevelMask;

exit:
    return error;
}

Error Frame::GetKeyIdMode(uint8_t &aKeyIdMode) const
{
    Error   error = kErrorNone;
    uint8_t index = FindSecurityHeaderIndex();

    VerifyOrExit(index != kInvalidIndex, error = kErrorParse);

    aKeyIdMode = mPsdu[index] & kKeyIdModeMask;

exit:
    return error;
}

Error Frame::GetFrameCounter(uint32_t &aFrameCounter) const
{
    Error   error = kErrorNone;
    uint8_t index = FindSecurityHeaderIndex();

    VerifyOrExit(index != kInvalidIndex, error = kErrorParse);

    // Security Control
    index += kSecurityControlSize;

    aFrameCounter = LittleEndian::ReadUint32(&mPsdu[index]);

exit:
    return error;
}

void Frame::SetFrameCounter(uint32_t aFrameCounter)
{
    uint8_t index = FindSecurityHeaderIndex();

    OT_ASSERT(index != kInvalidIndex);

    // Security Control
    index += kSecurityControlSize;

    LittleEndian::WriteUint32(aFrameCounter, &mPsdu[index]);

    static_cast<Mac::TxFrame *>(this)->SetIsHeaderUpdated(true);
}

const uint8_t *Frame::GetKeySource(void) const
{
    uint8_t index = FindSecurityHeaderIndex();

    OT_ASSERT(index != kInvalidIndex);

    return &mPsdu[index + kSecurityControlSize + kFrameCounterSize];
}

uint8_t Frame::GetKeySourceLength(uint8_t aKeyIdMode)
{
    uint8_t len = 0;

    switch (aKeyIdMode)
    {
    case kKeyIdMode0:
        len = kKeySourceSizeMode0;
        break;

    case kKeyIdMode1:
        len = kKeySourceSizeMode1;
        break;

    case kKeyIdMode2:
        len = kKeySourceSizeMode2;
        break;

    case kKeyIdMode3:
        len = kKeySourceSizeMode3;
        break;
    }

    return len;
}

void Frame::SetKeySource(const uint8_t *aKeySource)
{
    uint8_t keySourceLength;
    uint8_t index = FindSecurityHeaderIndex();

    OT_ASSERT(index != kInvalidIndex);

    keySourceLength = GetKeySourceLength(mPsdu[index] & kKeyIdModeMask);

    memcpy(&mPsdu[index + kSecurityControlSize + kFrameCounterSize], aKeySource, keySourceLength);
}

Error Frame::GetKeyId(uint8_t &aKeyId) const
{
    Error   error = kErrorNone;
    uint8_t keySourceLength;
    uint8_t index = FindSecurityHeaderIndex();

    VerifyOrExit(index != kInvalidIndex, error = kErrorParse);

    keySourceLength = GetKeySourceLength(mPsdu[index] & kKeyIdModeMask);

    aKeyId = mPsdu[index + kSecurityControlSize + kFrameCounterSize + keySourceLength];

exit:
    return error;
}

void Frame::SetKeyId(uint8_t aKeyId)
{
    uint8_t keySourceLength;
    uint8_t index = FindSecurityHeaderIndex();

    OT_ASSERT(index != kInvalidIndex);

    keySourceLength = GetKeySourceLength(mPsdu[index] & kKeyIdModeMask);

    mPsdu[index + kSecurityControlSize + kFrameCounterSize + keySourceLength] = aKeyId;
}

Error Frame::GetCommandId(uint8_t &aCommandId) const
{
    Error   error = kErrorNone;
    uint8_t index = FindPayloadIndex();

    VerifyOrExit(index != kInvalidIndex, error = kErrorParse);

    aCommandId = mPsdu[IsVersion2015() ? index : (index - 1)];

exit:
    return error;
}

Error Frame::SetCommandId(uint8_t aCommandId)
{
    Error   error = kErrorNone;
    uint8_t index = FindPayloadIndex();

    VerifyOrExit(index != kInvalidIndex, error = kErrorParse);

    mPsdu[IsVersion2015() ? index : (index - 1)] = aCommandId;

exit:
    return error;
}

bool Frame::IsDataRequestCommand(void) const
{
    bool    isDataRequest = false;
    uint8_t commandId;

    VerifyOrExit(GetType() == kTypeMacCmd);
    SuccessOrExit(GetCommandId(commandId));
    isDataRequest = (commandId == kMacCmdDataRequest);

exit:
    return isDataRequest;
}

uint8_t Frame::GetHeaderLength(void) const { return static_cast<uint8_t>(GetPayload() - mPsdu); }

uint8_t Frame::GetFooterLength(void) const
{
    uint8_t footerLength = static_cast<uint8_t>(GetFcsSize());
    uint8_t index        = FindSecurityHeaderIndex();

    VerifyOrExit(index != kInvalidIndex);
    footerLength += CalculateMicSize(mPsdu[index]);

exit:
    return footerLength;
}

uint8_t Frame::CalculateMicSize(uint8_t aSecurityControl)
{
    uint8_t micSize = 0;

    switch (aSecurityControl & kSecLevelMask)
    {
    case kSecurityNone:
    case kSecurityEnc:
        micSize = kMic0Size;
        break;

    case kSecurityMic32:
    case kSecurityEncMic32:
        micSize = kMic32Size;
        break;

    case kSecurityMic64:
    case kSecurityEncMic64:
        micSize = kMic64Size;
        break;

    case kSecurityMic128:
    case kSecurityEncMic128:
        micSize = kMic128Size;
        break;
    }

    return micSize;
}

uint16_t Frame::GetMaxPayloadLength(void) const { return GetMtu() - (GetHeaderLength() + GetFooterLength()); }

uint16_t Frame::GetPayloadLength(void) const { return mLength - (GetHeaderLength() + GetFooterLength()); }

void Frame::SetPayloadLength(uint16_t aLength) { mLength = GetHeaderLength() + GetFooterLength() + aLength; }

uint8_t Frame::SkipSecurityHeaderIndex(void) const
{
    uint8_t index = SkipAddrFieldIndex();

    VerifyOrExit(index != kInvalidIndex);

    if (GetSecurityEnabled())
    {
        uint8_t securityControl;
        uint8_t headerSize;

        VerifyOrExit(index < mLength, index = kInvalidIndex);
        securityControl = mPsdu[index];

        headerSize = CalculateSecurityHeaderSize(securityControl);
        VerifyOrExit(headerSize != kInvalidSize, index = kInvalidIndex);

        index += headerSize;

        VerifyOrExit(index <= mLength, index = kInvalidIndex);
    }

exit:
    return index;
}

uint8_t Frame::CalculateSecurityHeaderSize(uint8_t aSecurityControl)
{
    uint8_t size = kSecurityControlSize + kFrameCounterSize;

    VerifyOrExit((aSecurityControl & kSecLevelMask) != kSecurityNone, size = kInvalidSize);

    switch (aSecurityControl & kKeyIdModeMask)
    {
    case kKeyIdMode0:
        size += kKeySourceSizeMode0;
        break;

    case kKeyIdMode1:
        size += kKeySourceSizeMode1 + kKeyIndexSize;
        break;

    case kKeyIdMode2:
        size += kKeySourceSizeMode2 + kKeyIndexSize;
        break;

    case kKeyIdMode3:
        size += kKeySourceSizeMode3 + kKeyIndexSize;
        break;
    }

exit:
    return size;
}

uint8_t Frame::SkipAddrFieldIndex(void) const
{
    uint8_t index;

    VerifyOrExit(kFcfSize + kDsnSize + GetFcsSize() <= mLength, index = kInvalidIndex);

    index = CalculateAddrFieldSize(GetFrameControlField());

exit:
    return index;
}

uint8_t Frame::CalculateAddrFieldSize(uint16_t aFcf)
{
    uint8_t size = kFcfSize + kDsnSize;

    // This static method calculates the size (number of bytes) of
    // Address header field for a given Frame Control `aFcf` value.
    // The size includes the Frame Control and Sequence Number fields
    // along with Destination and Source PAN ID and Short/Extended
    // Addresses. If the `aFcf` is not valid, this method returns
    // `kInvalidSize`.

    if (IsDstPanIdPresent(aFcf))
    {
        size += sizeof(PanId);
    }

    switch (aFcf & kFcfDstAddrMask)
    {
    case kFcfDstAddrNone:
        break;

    case kFcfDstAddrShort:
        size += sizeof(ShortAddress);
        break;

    case kFcfDstAddrExt:
        size += sizeof(ExtAddress);
        break;

    default:
        ExitNow(size = kInvalidSize);
    }

    if (IsSrcPanIdPresent(aFcf))
    {
        size += sizeof(PanId);
    }

    switch (aFcf & kFcfSrcAddrMask)
    {
    case kFcfSrcAddrNone:
        break;

    case kFcfSrcAddrShort:
        size += sizeof(ShortAddress);
        break;

    case kFcfSrcAddrExt:
        size += sizeof(ExtAddress);
        break;

    default:
        ExitNow(size = kInvalidSize);
    }

exit:
    return size;
}

uint8_t Frame::FindPayloadIndex(void) const
{
    // We use `uint16_t` for `index` to handle its potential roll-over
    // while parsing and verifying Header IE(s).

    uint16_t index = SkipSecurityHeaderIndex();

    VerifyOrExit(index != kInvalidIndex);

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    if (IsIePresent())
    {
        uint8_t footerLength = GetFooterLength();

        do
        {
            const HeaderIe *ie = reinterpret_cast<const HeaderIe *>(&mPsdu[index]);

            index += sizeof(HeaderIe);
            VerifyOrExit(index + footerLength <= mLength, index = kInvalidIndex);

            index += ie->GetLength();
            VerifyOrExit(index + footerLength <= mLength, index = kInvalidIndex);

            if (ie->GetId() == Termination2Ie::kHeaderIeId)
            {
                break;
            }

            // If the `index + footerLength == mLength`, we exit the `while()`
            // loop. This covers the case where frame contains one or more
            // Header IEs but no data payload. In this case, spec does not
            // require Header IE termination to be included (it is optional)
            // since the end of frame can be determined from frame length and
            // footer length.

        } while (index + footerLength < mLength);

        // Assume no Payload IE in current implementation
    }
#endif // OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT

    if (!IsVersion2015() && (GetFrameControlField() & kFcfFrameTypeMask) == kTypeMacCmd)
    {
        index += kCommandIdSize;
    }

exit:
    return static_cast<uint8_t>(index);
}

const uint8_t *Frame::GetPayload(void) const
{
    uint8_t        index = FindPayloadIndex();
    const uint8_t *payload;

    VerifyOrExit(index != kInvalidIndex, payload = nullptr);
    payload = &mPsdu[index];

exit:
    return payload;
}

const uint8_t *Frame::GetFooter(void) const { return mPsdu + mLength - GetFooterLength(); }

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
uint8_t Frame::FindHeaderIeIndex(void) const
{
    uint8_t index;

    VerifyOrExit(IsIePresent(), index = kInvalidIndex);

    index = SkipSecurityHeaderIndex();

exit:
    return index;
}

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
template <typename IeType> Error Frame::AppendHeaderIeAt(uint8_t &aIndex)
{
    Error error = kErrorNone;

    SuccessOrExit(error = InitIeHeaderAt(aIndex, IeType::kHeaderIeId, IeType::kIeContentSize));

    InitIeContentAt<IeType>(aIndex);

exit:
    return error;
}

Error Frame::InitIeHeaderAt(uint8_t &aIndex, uint8_t ieId, uint8_t ieContentSize)
{
    Error error = kErrorNone;

    SetIePresent(true);

    if (aIndex == 0)
    {
        aIndex = FindHeaderIeIndex();
    }

    VerifyOrExit(aIndex != kInvalidIndex, error = kErrorNotFound);

    reinterpret_cast<HeaderIe *>(mPsdu + aIndex)->Init(ieId, ieContentSize);
    aIndex += sizeof(HeaderIe);

    mLength += sizeof(HeaderIe) + ieContentSize;
exit:
    return error;
}

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
template <> void Frame::InitIeContentAt<TimeIe>(uint8_t &aIndex)
{
    reinterpret_cast<TimeIe *>(mPsdu + aIndex)->Init();
    aIndex += sizeof(TimeIe);
}
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
template <> void Frame::InitIeContentAt<CslIe>(uint8_t &aIndex) { aIndex += sizeof(CslIe); }
#endif

template <> void Frame::InitIeContentAt<Termination2Ie>(uint8_t &aIndex) { OT_UNUSED_VARIABLE(aIndex); }
#endif // OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT

const uint8_t *Frame::GetHeaderIe(uint8_t aIeId) const
{
    uint8_t        index        = FindHeaderIeIndex();
    uint8_t        payloadIndex = FindPayloadIndex();
    const uint8_t *header       = nullptr;

    // `FindPayloadIndex()` verifies that Header IE(s) in frame (if present)
    // are well-formed.

    VerifyOrExit((index != kInvalidIndex) && (payloadIndex != kInvalidIndex));

    while (index <= payloadIndex)
    {
        const HeaderIe *ie = reinterpret_cast<const HeaderIe *>(&mPsdu[index]);

        if (ie->GetId() == aIeId)
        {
            header = &mPsdu[index];
            ExitNow();
        }

        index += sizeof(HeaderIe) + ie->GetLength();
    }

exit:
    return header;
}

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
const uint8_t *Frame::GetThreadIe(uint8_t aSubType) const
{
    uint8_t        index        = FindHeaderIeIndex();
    uint8_t        payloadIndex = FindPayloadIndex();
    const uint8_t *header       = nullptr;

    // `FindPayloadIndex()` verifies that Header IE(s) in frame (if present)
    // are well-formed.
    VerifyOrExit((index != kInvalidIndex) && (payloadIndex != kInvalidIndex));

    while (index <= payloadIndex)
    {
        const HeaderIe *ie = reinterpret_cast<const HeaderIe *>(&mPsdu[index]);

        if (ie->GetId() == VendorIeHeader::kHeaderIeId)
        {
            const VendorIeHeader *vendorIe =
                reinterpret_cast<const VendorIeHeader *>(reinterpret_cast<const uint8_t *>(ie) + sizeof(HeaderIe));
            if (vendorIe->GetVendorOui() == ThreadIe::kVendorOuiThreadCompanyId && vendorIe->GetSubType() == aSubType)
            {
                header = &mPsdu[index];
                ExitNow();
            }
        }

        index += sizeof(HeaderIe) + ie->GetLength();
    }

exit:
    return header;
}
#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

#endif // OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
void Frame::SetCslIe(uint16_t aCslPeriod, uint16_t aCslPhase)
{
    uint8_t *cur = GetHeaderIe(CslIe::kHeaderIeId);
    CslIe   *csl;

    VerifyOrExit(cur != nullptr);

    csl = reinterpret_cast<CslIe *>(cur + sizeof(HeaderIe));
    csl->SetPeriod(aCslPeriod);
    csl->SetPhase(aCslPhase);
exit:
    return;
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
void Frame::SetEnhAckProbingIe(const uint8_t *aValue, uint8_t aLen)
{
    uint8_t *cur = GetThreadIe(ThreadIe::kEnhAckProbingIe);

    OT_ASSERT(cur != nullptr);

    memcpy(cur + sizeof(HeaderIe) + sizeof(VendorIeHeader), aValue, aLen);
}
#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
const TimeIe *Frame::GetTimeIe(void) const
{
    const TimeIe  *timeIe = nullptr;
    const uint8_t *cur    = nullptr;

    cur = GetHeaderIe(VendorIeHeader::kHeaderIeId);
    VerifyOrExit(cur != nullptr);

    cur += sizeof(HeaderIe);

    timeIe = reinterpret_cast<const TimeIe *>(cur);
    VerifyOrExit(timeIe->GetVendorOui() == TimeIe::kVendorOuiNest, timeIe = nullptr);
    VerifyOrExit(timeIe->GetSubType() == TimeIe::kVendorIeTime, timeIe = nullptr);

exit:
    return timeIe;
}
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

#if OPENTHREAD_CONFIG_MULTI_RADIO
uint16_t Frame::GetMtu(void) const
{
    uint16_t mtu = 0;

    switch (GetRadioType())
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    case kRadioTypeIeee802154:
        mtu = OT_RADIO_FRAME_MAX_SIZE;
        break;
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    case kRadioTypeTrel:
        mtu = Trel::Link::kMtuSize;
        break;
#endif
    }

    return mtu;
}

uint8_t Frame::GetFcsSize(void) const
{
    uint8_t fcsSize = 0;

    switch (GetRadioType())
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    case kRadioTypeIeee802154:
        fcsSize = k154FcsSize;
        break;
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    case kRadioTypeTrel:
        fcsSize = Trel::Link::kFcsSize;
        break;
#endif
    }

    return fcsSize;
}

#elif OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
uint16_t Frame::GetMtu(void) const { return Trel::Link::kMtuSize; }

uint8_t Frame::GetFcsSize(void) const { return Trel::Link::kFcsSize; }
#endif

// Explicit instantiation
#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
template Error Frame::AppendHeaderIeAt<TimeIe>(uint8_t &aIndex);
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
template Error Frame::AppendHeaderIeAt<CslIe>(uint8_t &aIndex);
#endif
template Error Frame::AppendHeaderIeAt<Termination2Ie>(uint8_t &aIndex);
#endif

void TxFrame::CopyFrom(const TxFrame &aFromFrame)
{
    uint8_t       *psduBuffer   = mPsdu;
    otRadioIeInfo *ieInfoBuffer = mInfo.mTxInfo.mIeInfo;
#if OPENTHREAD_CONFIG_MULTI_RADIO
    uint8_t radioType = mRadioType;
#endif

    memcpy(this, &aFromFrame, sizeof(Frame));

    // Set the original buffer pointers (and link type) back on
    // the frame (which were overwritten by above `memcpy()`).

    mPsdu                 = psduBuffer;
    mInfo.mTxInfo.mIeInfo = ieInfoBuffer;

#if OPENTHREAD_CONFIG_MULTI_RADIO
    mRadioType = radioType;
#endif

    memcpy(mPsdu, aFromFrame.mPsdu, aFromFrame.mLength);

    // mIeInfo may be null when TIME_SYNC is not enabled.
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    memcpy(mInfo.mTxInfo.mIeInfo, aFromFrame.mInfo.mTxInfo.mIeInfo, sizeof(otRadioIeInfo));
#endif

#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (mRadioType != aFromFrame.GetRadioType())
    {
        // Frames associated with different radio link types can have
        // different FCS size. We adjust the PSDU length after the
        // copy to account for this.

        SetLength(aFromFrame.GetLength() - aFromFrame.GetFcsSize() + GetFcsSize());
    }
#endif
}

void TxFrame::ProcessTransmitAesCcm(const ExtAddress &aExtAddress)
{
#if OPENTHREAD_RADIO && !OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE
    OT_UNUSED_VARIABLE(aExtAddress);
#else
    uint32_t       frameCounter = 0;
    uint8_t        securityLevel;
    uint8_t        nonce[Crypto::AesCcm::kNonceSize];
    uint8_t        tagLength;
    Crypto::AesCcm aesCcm;

    VerifyOrExit(GetSecurityEnabled());

    SuccessOrExit(GetSecurityLevel(securityLevel));
    SuccessOrExit(GetFrameCounter(frameCounter));

    Crypto::AesCcm::GenerateNonce(aExtAddress, frameCounter, securityLevel, nonce);

    aesCcm.SetKey(GetAesKey());
    tagLength = GetFooterLength() - GetFcsSize();

    aesCcm.Init(GetHeaderLength(), GetPayloadLength(), tagLength, nonce, sizeof(nonce));
    aesCcm.Header(GetHeader(), GetHeaderLength());
    aesCcm.Payload(GetPayload(), GetPayload(), GetPayloadLength(), Crypto::AesCcm::kEncrypt);
    aesCcm.Finalize(GetFooter());

    SetIsSecurityProcessed(true);

exit:
    return;
#endif // OPENTHREAD_RADIO && !OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE
}

void TxFrame::GenerateImmAck(const RxFrame &aFrame, bool aIsFramePending)
{
    uint16_t fcf = static_cast<uint16_t>(kTypeAck) | aFrame.GetVersion();

    mChannel = aFrame.mChannel;
    memset(&mInfo.mTxInfo, 0, sizeof(mInfo.mTxInfo));

    if (aIsFramePending)
    {
        fcf |= kFcfFramePending;
    }
    LittleEndian::WriteUint16(fcf, mPsdu);

    mPsdu[kSequenceIndex] = aFrame.GetSequence();

    mLength = kImmAckLength;
}

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
Error TxFrame::GenerateEnhAck(const RxFrame &aRxFrame, bool aIsFramePending, const uint8_t *aIeData, uint8_t aIeLength)
{
    Error     error = kErrorNone;
    Address   address;
    PanId     panId;
    Addresses addrs;
    PanIds    panIds;
    uint8_t   securityLevel = kSecurityNone;
    uint8_t   keyIdMode     = kKeyIdMode0;

    // Validate the received frame.

    VerifyOrExit(aRxFrame.IsVersion2015(), error = kErrorParse);
    VerifyOrExit(aRxFrame.GetAckRequest(), error = kErrorParse);

    // Check `aRxFrame` has a valid destination address. The ack frame
    // will not use this as its source though and will always use no
    // source address.

    SuccessOrExit(error = aRxFrame.GetDstAddr(address));
    VerifyOrExit(!address.IsNone() && !address.IsBroadcast(), error = kErrorParse);

    // Check `aRxFrame` has a valid source, which is then used as
    // ack frames destination.

    SuccessOrExit(error = aRxFrame.GetSrcAddr(addrs.mDestination));
    VerifyOrExit(!addrs.mDestination.IsNone(), error = kErrorParse);

    if (aRxFrame.GetSecurityEnabled())
    {
        SuccessOrExit(error = aRxFrame.GetSecurityLevel(securityLevel));
        VerifyOrExit(securityLevel == kSecurityEncMic32, error = kErrorParse);

        SuccessOrExit(error = aRxFrame.GetKeyIdMode(keyIdMode));
    }

    if (aRxFrame.IsSrcPanIdPresent())
    {
        SuccessOrExit(error = aRxFrame.GetSrcPanId(panId));
        panIds.SetDestination(panId);
    }
    else if (aRxFrame.IsDstPanIdPresent())
    {
        SuccessOrExit(error = aRxFrame.GetDstPanId(panId));
        panIds.SetDestination(panId);
    }

    // Prepare the ack frame

    mChannel = aRxFrame.mChannel;
    memset(&mInfo.mTxInfo, 0, sizeof(mInfo.mTxInfo));

    InitMacHeader(kTypeAck, kVersion2015, addrs, panIds, static_cast<SecurityLevel>(securityLevel),
                  static_cast<KeyIdMode>(keyIdMode));

    SetFramePending(aIsFramePending);
    SetIePresent(aIeLength != 0);
    SetSequence(aRxFrame.GetSequence());

    if (aRxFrame.GetSecurityEnabled())
    {
        uint8_t keyId;

        SuccessOrExit(error = aRxFrame.GetKeyId(keyId));
        SetKeyId(keyId);
    }

    if (aIeLength > 0)
    {
        OT_ASSERT(aIeData != nullptr);
        memcpy(&mPsdu[FindHeaderIeIndex()], aIeData, aIeLength);
        mLength += aIeLength;
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2

Error RxFrame::ProcessReceiveAesCcm(const ExtAddress &aExtAddress, const KeyMaterial &aMacKey)
{
#if OPENTHREAD_RADIO
    OT_UNUSED_VARIABLE(aExtAddress);
    OT_UNUSED_VARIABLE(aMacKey);

    return kErrorNone;
#else
    Error          error        = kErrorSecurity;
    uint32_t       frameCounter = 0;
    uint8_t        securityLevel;
    uint8_t        nonce[Crypto::AesCcm::kNonceSize];
    uint8_t        tag[kMaxMicSize];
    uint8_t        tagLength;
    Crypto::AesCcm aesCcm;

    VerifyOrExit(GetSecurityEnabled(), error = kErrorNone);

    SuccessOrExit(GetSecurityLevel(securityLevel));
    SuccessOrExit(GetFrameCounter(frameCounter));

    Crypto::AesCcm::GenerateNonce(aExtAddress, frameCounter, securityLevel, nonce);

    aesCcm.SetKey(aMacKey);
    tagLength = GetFooterLength() - GetFcsSize();

    aesCcm.Init(GetHeaderLength(), GetPayloadLength(), tagLength, nonce, sizeof(nonce));
    aesCcm.Header(GetHeader(), GetHeaderLength());
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    aesCcm.Payload(GetPayload(), GetPayload(), GetPayloadLength(), Crypto::AesCcm::kDecrypt);
#else
    // For fuzz tests, execute AES but do not alter the payload
    uint8_t fuzz[OT_RADIO_FRAME_MAX_SIZE];
    aesCcm.Payload(fuzz, GetPayload(), GetPayloadLength(), Crypto::AesCcm::kDecrypt);
#endif
    aesCcm.Finalize(tag);

#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    VerifyOrExit(memcmp(tag, GetFooter(), tagLength) == 0);
#endif

    error = kErrorNone;

exit:
    return error;
#endif // OPENTHREAD_RADIO
}

// LCOV_EXCL_START

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE)

Frame::InfoString Frame::ToInfoString(void) const
{
    InfoString string;
    uint8_t    commandId, type;
    Address    src, dst;

    string.Append("len:%d, seqnum:%d, type:", mLength, GetSequence());

    type = GetType();

    switch (type)
    {
    case kTypeBeacon:
        string.Append("Beacon");
        break;

    case kTypeData:
        string.Append("Data");
        break;

    case kTypeAck:
        string.Append("Ack");
        break;

    case kTypeMacCmd:
        if (GetCommandId(commandId) != kErrorNone)
        {
            commandId = 0xff;
        }

        switch (commandId)
        {
        case kMacCmdDataRequest:
            string.Append("Cmd(DataReq)");
            break;

        case kMacCmdBeaconRequest:
            string.Append("Cmd(BeaconReq)");
            break;

        default:
            string.Append("Cmd(%d)", commandId);
            break;
        }

        break;

    default:
        string.Append("%d", type);
        break;
    }

    IgnoreError(GetSrcAddr(src));
    IgnoreError(GetDstAddr(dst));

    string.Append(", src:%s, dst:%s, sec:%s, ackreq:%s", src.ToString().AsCString(), dst.ToString().AsCString(),
                  ToYesNo(GetSecurityEnabled()), ToYesNo(GetAckRequest()));

#if OPENTHREAD_CONFIG_MULTI_RADIO
    string.Append(", radio:%s", RadioTypeToString(GetRadioType()));
#endif

    return string;
}

#endif // #if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE)

// LCOV_EXCL_STOP

} // namespace Mac
} // namespace ot
