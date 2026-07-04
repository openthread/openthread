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
#include "common/num_utils.hpp"
#include "radio/trel_link.hpp"
#if OPENTHREAD_FTD || OPENTHREAD_MTD || OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE || \
    (OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT && OPENTHREAD_CONFIG_MAC_SOFTWARE_RETX_SECURITY_ENABLE)
#include "crypto/aes_ccm.hpp"
#endif

namespace ot {
namespace Mac {

void TxFrame::BuildInfo::PrepareHeadersIn(TxFrame &aTxFrame) const
{
    uint16_t     fcf;
    FrameBuilder builder;
    uint8_t      micSize = 0;

    fcf = static_cast<uint16_t>(mType) | static_cast<uint16_t>(mVersion);

    fcf |= DetermineFcfAddrType(mAddrs.mSource, kFcfSrcAddrShift);
    fcf |= DetermineFcfAddrType(mAddrs.mDestination, kFcfDstAddrShift);

    if (!mAddrs.mDestination.IsNone() && !mAddrs.mDestination.IsBroadcast() && (mType != kTypeAck))
    {
        fcf |= kFcfAckRequest;
    }

    fcf |= (mSecurityLevel != kSecurityNone) ? kFcfSecurityEnabled : 0;

    // PAN ID compression

    switch (mVersion)
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

        if (!mAddrs.mSource.IsNone() && !mAddrs.mDestination.IsNone() &&
            (mPanIds.GetSource() == mPanIds.GetDestination()))
        {
            fcf |= kFcfPanidCompression;
        }

        // Sequence Number Suppression bit was reserved, and must not be set on initialization.
        OT_ASSERT(!mSuppressSequence);
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
        //
        // This table shows the combination of flags allowed in an encoded MAC
        // header. Regarding rows 9-14, when both Source and Destination
        // Address fields are present and at least one uses a short address
        // format, then if the source and destination PAN IDs are equal, PAN
        // ID compression is set to 1.

        if (mAddrs.mDestination.IsNone())
        {
            // Dst addr not present - rows 1,2,5,6.

            if ((mAddrs.mSource.IsNone() && mPanIds.IsDestinationPresent()) ||                               // Row 2.
                (!mAddrs.mSource.IsNone() && !mPanIds.IsDestinationPresent() && !mPanIds.IsSourcePresent())) // Row 6.
            {
                fcf |= kFcfPanidCompression;
            }

            break;
        }

        if (mAddrs.mSource.IsNone())
        {
            // Dst addr present, Src addr not present - rows 3,4.

            if (!mPanIds.IsDestinationPresent()) // Row 4.
            {
                fcf |= kFcfPanidCompression;
            }

            break;
        }

        // Both addresses are present - rows 7 to 14.

        if (mAddrs.mSource.IsExtended() && mAddrs.mDestination.IsExtended())
        {
            // Both addresses are extended - rows 7,8.

            if (mPanIds.IsDestinationPresent()) // Row 7.
            {
                break;
            }
        }
        else if (mPanIds.GetSource() != mPanIds.GetDestination()) // Rows 9-14.
        {
            break;
        }

        fcf |= kFcfPanidCompression;

        break;
    }

    if (mSuppressSequence)
    {
        fcf |= kFcfSequenceSuppression;
    }

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    fcf |= (mAppendTimeIe ? kFcfIePresent : 0);
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    fcf |= (mAppendCslIe ? kFcfIePresent : 0);
#endif
#endif

    builder.Init(aTxFrame.mPsdu, aTxFrame.GetMtu());
    IgnoreError(builder.AppendUint<kLittleEndian>(fcf));

    if (IsSequencePresent(fcf))
    {
        builder.Append<uint8_t>(); // Place holder for seq number
    }

    if (IsDstPanIdPresent(fcf))
    {
        IgnoreError(builder.AppendUint<kLittleEndian>(mPanIds.GetDestination()));
    }

    IgnoreError(builder.AppendMacAddress(mAddrs.mDestination));

    if (IsSrcPanIdPresent(fcf))
    {
        IgnoreError(builder.AppendUint<kLittleEndian>(mPanIds.GetSource()));
    }

    IgnoreError(builder.AppendMacAddress(mAddrs.mSource));

    aTxFrame.mLength = builder.GetLength();

    if (mSecurityLevel != kSecurityNone)
    {
        uint8_t secCtl = static_cast<uint8_t>(mSecurityLevel) | static_cast<uint8_t>(mKeyIdMode);

        IgnoreError(builder.AppendUint8(secCtl));
        builder.AppendLength(CalculateSecurityHeaderSize(secCtl) - sizeof(secCtl));

        micSize = CalculateMicSize(secCtl);
    }

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if (mAppendTimeIe)
    {
        builder.Append<TimeIe>()->Init();
    }
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (mAppendCslIe)
    {
        builder.Append<CslIe>()->Init();
        aTxFrame.SetCslIePresent(true);
    }
#endif

    if ((fcf & kFcfIePresent) && ((mType == kTypeMacCmd) || !mEmptyPayload))
    {
        builder.Append<Termination2Ie>()->Init();
    }

#endif //  OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT

    if (mType == kTypeMacCmd)
    {
        IgnoreError(builder.AppendUint8(mCommandId));
    }

    builder.AppendLength(micSize + aTxFrame.GetFcsSize());

    aTxFrame.mLength = builder.GetLength();
}

Error Frame::ValidatePsdu(void) const
{
    Error   error = kErrorNone;
    uint8_t index = FindPayloadIndex();

    VerifyOrExit(index != kInvalidIndex, error = kErrorParse);

    if (IsMacCommand() && IsVersion2015())
    {
        // The treatment of the Command ID field in a MAC command frame
        // is version-dependent. In the 2015 spec, it is part of the
        // encrypted payload, while in earlier versions, it is part of
        // the MAC header.
        //
        // `FindPayloadIndex()` accounts for this difference and returns
        // the starting index of the payload. To correctly validate a
        // 2015 frame, we must ensure it is long enough to contain the
        // Command ID, so we include its size in the length check.

        index += kCommandIdSize;
    }

    VerifyOrExit((index + GetFooterLength()) <= mLength, error = kErrorParse);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
bool Frame::IsWakeupFrame(void) const
{
    // Placeholder implementation following removal of legacy Multipurpose frame format.
    return false;
}
#endif

void Frame::UpdateFcfFlag(bool aSet, uint16_t aBitFlag)
{
    uint16_t fcf = GetFrameControlField();

    if (aSet)
    {
        fcf |= aBitFlag;
    }
    else
    {
        fcf &= ~aBitFlag;
    }

    SetFrameControlField(fcf);
}

uint8_t Frame::SkipSequenceIndex(void) const
{
    uint16_t fcf   = GetFrameControlField();
    uint8_t  index = kFcfSize;

    if (IsSequencePresent(fcf))
    {
        index += kDsnSize;
    }

    return index;
}

uint8_t Frame::FindDstPanIdIndex(void) const
{
    uint8_t index;

    VerifyOrExit(IsDstPanIdPresent(), index = kInvalidIndex);

    index = SkipSequenceIndex();

exit:
    return index;
}

bool Frame::IsDstPanIdPresent(uint16_t aFcf)
{
    bool present;

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
            present = true;
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

uint8_t Frame::GetSequence(void) const
{
    OT_ASSERT(IsSequencePresent());

    return GetPsdu()[kFcfSize];
}

void Frame::SetSequence(uint8_t aSequence)
{
    OT_ASSERT(IsSequencePresent());

    GetPsdu()[kFcfSize] = aSequence;
}

uint8_t Frame::FindDstAddrIndex(void) const { return SkipSequenceIndex() + (IsDstPanIdPresent() ? sizeof(PanId) : 0); }

Error Frame::GetDstAddr(Address &aAddress) const
{
    Error   error = kErrorNone;
    uint8_t index = FindDstAddrIndex();

    VerifyOrExit(index != kInvalidIndex, error = kErrorParse);

    switch (GetFcfDstAddr(GetFrameControlField()))
    {
    case kFcfAddrShort:
        aAddress.SetShort(LittleEndian::ReadUint16(&mPsdu[index]));
        break;

    case kFcfAddrExt:
        aAddress.SetExtended(&mPsdu[index], ExtAddress::kReverseByteOrder);
        break;

    default:
        aAddress.SetNone();
        break;
    }

exit:
    return error;
}

uint8_t Frame::FindSrcPanIdIndex(void) const
{
    uint16_t fcf = GetFrameControlField();
    uint8_t  index;

    VerifyOrExit(IsSrcPanIdPresent(fcf), index = kInvalidIndex);

    index = SkipSequenceIndex();

    if (IsDstPanIdPresent(fcf))
    {
        index += sizeof(PanId);
    }

    switch (GetFcfDstAddr(fcf))
    {
    case kFcfAddrShort:
        index += sizeof(ShortAddress);
        break;

    case kFcfAddrExt:
        index += sizeof(ExtAddress);
        break;
    }

exit:
    return index;
}

bool Frame::IsSrcPanIdPresent(uint16_t aFcf)
{
    bool present;

    if (IsVersion2015(aFcf) && ((aFcf & (kFcfDstAddrMask | kFcfSrcAddrMask)) == (kFcfDstAddrExt | kFcfSrcAddrExt)))
    {
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

        present = false;
    }
    else
    {
        present = IsSrcAddrPresent(aFcf) && ((aFcf & kFcfPanidCompression) == 0);
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

uint8_t Frame::FindSrcAddrIndex(void) const
{
    uint16_t fcf   = GetFrameControlField();
    uint8_t  index = SkipSequenceIndex();

    if (IsDstPanIdPresent(fcf))
    {
        index += sizeof(PanId);
    }

    switch (GetFcfDstAddr(fcf))
    {
    case kFcfAddrShort:
        index += sizeof(ShortAddress);
        break;

    case kFcfAddrExt:
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

    switch (GetFcfSrcAddr(fcf))
    {
    case kFcfAddrShort:
        aAddress.SetShort(LittleEndian::ReadUint16(&mPsdu[index]));
        break;

    case kFcfAddrExt:
        aAddress.SetExtended(&mPsdu[index], ExtAddress::kReverseByteOrder);
        break;

    case kFcfAddrNone:
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

Error Frame::GetSecurityControlField(uint8_t &aSecurityControlField) const
{
    Error   error = kErrorNone;
    uint8_t index = FindSecurityHeaderIndex();

    VerifyOrExit(index != kInvalidIndex, error = kErrorParse);

    aSecurityControlField = mPsdu[index];

exit:
    return error;
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

    static_cast<TxFrame *>(this)->SetIsHeaderUpdated(true);
}

const uint8_t *Frame::GetKeySource(void) const
{
    uint8_t index = FindSecurityHeaderIndex();

    OT_ASSERT(index != kInvalidIndex);

    return &mPsdu[index + kSecurityControlSize + kFrameCounterSize];
}

uint8_t Frame::CalculateKeySourceSize(uint8_t aSecurityControl)
{
    uint8_t size = 0;

    switch (aSecurityControl & kKeyIdModeMask)
    {
    case kKeyIdMode0:
        size = kKeySourceSizeMode0;
        break;

    case kKeyIdMode1:
        size = kKeySourceSizeMode1;
        break;

    case kKeyIdMode2:
        size = kKeySourceSizeMode2;
        break;

    case kKeyIdMode3:
        size = kKeySourceSizeMode3;
        break;
    }

    return size;
}

void Frame::SetKeySource(const uint8_t *aKeySource)
{
    uint8_t keySourceSize;
    uint8_t index = FindSecurityHeaderIndex();

    OT_ASSERT(index != kInvalidIndex);

    keySourceSize = CalculateKeySourceSize(mPsdu[index]);

    memcpy(&mPsdu[index + kSecurityControlSize + kFrameCounterSize], aKeySource, keySourceSize);
}

Error Frame::GetKeyId(uint8_t &aKeyId) const
{
    Error   error = kErrorNone;
    uint8_t keySourceSize;
    uint8_t index = FindSecurityHeaderIndex();

    VerifyOrExit(index != kInvalidIndex, error = kErrorParse);

    keySourceSize = CalculateKeySourceSize(mPsdu[index]);

    aKeyId = mPsdu[index + kSecurityControlSize + kFrameCounterSize + keySourceSize];

exit:
    return error;
}

void Frame::SetKeyId(uint8_t aKeyId)
{
    uint8_t keySourceSize;
    uint8_t index = FindSecurityHeaderIndex();

    OT_ASSERT(index != kInvalidIndex);

    keySourceSize = CalculateKeySourceSize(mPsdu[index]);

    mPsdu[index + kSecurityControlSize + kFrameCounterSize + keySourceSize] = aKeyId;
}

Error Frame::GetCommandId(uint8_t &aCommandId) const
{
    Error   error = kErrorNone;
    uint8_t index = FindPayloadIndex();

    VerifyOrExit(index != kInvalidIndex, error = kErrorParse);

    // The treatment of the Command ID field in a MAC command frame
    // is version-dependent. In the 2015 spec, it is part of the
    // encrypted payload, while in earlier versions, it is part of
    // the MAC header. `FindPayloadIndex() accounts for both cases.

    if (!IsVersion2015())
    {
        index -= kCommandIdSize;
    }

    VerifyOrExit(index + kCommandIdSize + GetFooterLength() <= mLength, error = kErrorParse);
    aCommandId = mPsdu[index];

exit:
    return error;
}

bool Frame::IsDataRequestCommand(void) const
{
    bool    isDataRequest = false;
    uint8_t commandId;

    VerifyOrExit(IsMacCommand());
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

uint16_t Frame::DetermineFcfAddrType(const Address &aAddress, uint16_t aBitShift)
{
    // Determines the FCF address type for a given `aAddress`. The
    // result will be bit-shifted using `aBitShift` value which
    // correspond to whether address is the source or destination
    // and whether the frame uses the general format or is a
    // multipurpose frame

    uint16_t fcfAddrType = kFcfAddrNone;

    switch (aAddress.GetType())
    {
    case Address::kTypeNone:
        break;
    case Address::kTypeShort:
        fcfAddrType = kFcfAddrShort;
        break;
    case Address::kTypeExtended:
        fcfAddrType = kFcfAddrExt;
        break;
    }

    fcfAddrType <<= aBitShift;

    return fcfAddrType;
}

uint8_t Frame::CalculateSecurityHeaderSize(uint8_t aSecurityControl)
{
    uint8_t size;

    VerifyOrExit((aSecurityControl & kSecLevelMask) != kSecurityNone, size = kInvalidSize);

    size = kSecurityControlSize + kFrameCounterSize + CalculateKeySourceSize(aSecurityControl);

    if ((aSecurityControl & kKeyIdModeMask) != kKeyIdMode0)
    {
        size += kKeyIndexSize;
    }

exit:
    return size;
}

uint8_t Frame::SkipAddrFieldIndex(void) const
{
    uint8_t index;

    VerifyOrExit(kFcfSize + GetFcsSize() <= mLength, index = kInvalidIndex);

    index = CalculateAddrFieldSize(GetFrameControlField());

exit:
    return index;
}

uint8_t Frame::CalculateAddrFieldSize(uint16_t aFcf)
{
    uint8_t size = kFcfSize + (IsSequencePresent(aFcf) ? kDsnSize : 0);

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

    switch (GetFcfDstAddr(aFcf))
    {
    case kFcfAddrNone:
        break;

    case kFcfAddrShort:
        size += sizeof(ShortAddress);
        break;

    case kFcfAddrExt:
        size += sizeof(ExtAddress);
        break;

    default:
        ExitNow(size = kInvalidSize);
    }

    if (IsSrcPanIdPresent(aFcf))
    {
        size += sizeof(PanId);
    }

    switch (GetFcfSrcAddr(aFcf))
    {
    case kFcfAddrNone:
        break;

    case kFcfAddrShort:
        size += sizeof(ShortAddress);
        break;

    case kFcfAddrExt:
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
            const HeaderIe *ie;

            VerifyOrExit(index + footerLength + sizeof(HeaderIe) <= mLength, index = kInvalidIndex);

            ie = reinterpret_cast<const HeaderIe *>(&mPsdu[index]);
            index += ie->GetSize();

            VerifyOrExit(index + footerLength <= mLength, index = kInvalidIndex);

            if (ie->GetId() == Termination2Ie::kId)
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

    if (IsMacCommand() && !IsVersion2015())
    {
        // The treatment of the Command ID field in a MAC command frame
        // is version-dependent. In IEEE 802.15.4-2015, it is part of
        // the payload and therefore encrypted. In earlier versions, it
        // is part of the MAC header and not encrypted.
        //
        // This adjusts the index to point to the start of the payload
        // for pre-2015 frames. The `GetCommandId()` method also
        // accounts for this version-specific difference.

        index += kCommandIdSize;
    }

exit:
    return (index <= kMaxPsduSize) ? static_cast<uint8_t>(index) : kInvalidIndex;
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

const HeaderIe *Frame::FindHeaderIe(HeaderIeMatcher aMatcher) const
{
    uint16_t        index        = FindHeaderIeIndex();
    uint16_t        payloadIndex = FindPayloadIndex();
    const HeaderIe *matchedIe    = nullptr;

    // `FindPayloadIndex()` verifies that Header IE(s) in frame (if present)
    // are well-formed.

    VerifyOrExit((index != kInvalidIndex) && (payloadIndex != kInvalidIndex));

    while (index < payloadIndex)
    {
        const HeaderIe *ie = reinterpret_cast<const HeaderIe *>(&mPsdu[index]);

        if (aMatcher(*ie))
        {
            matchedIe = ie;
            ExitNow();
        }

        index += ie->GetSize();
    }

exit:
    return matchedIe;
}

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
void Frame::UpdateCslIe(uint16_t aCslPeriod, uint16_t aCslPhase)
{
    CslIe *csl = Find<CslIe>();

    VerifyOrExit(csl != nullptr);

    csl->SetPeriod(aCslPeriod);
    csl->SetPhase(aCslPhase);

exit:
    return;
}
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
void Frame::UpdateEnhAckProbingIe(const uint8_t *aData, uint8_t aLen)
{
    LinkMetricsProbingIe *probingIe = Find<LinkMetricsProbingIe>();

    VerifyOrExit(probingIe != nullptr);

    VerifyOrExit(aLen >= probingIe->GetMetricsDataLen());
    probingIe->WriteMetricsDataFrom(aData);

exit:
    return;
}
#endif

#endif // OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT

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
#if OPENTHREAD_FTD || OPENTHREAD_MTD || OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE
    uint32_t              frameCounter = 0;
    uint8_t               securityLevel;
    Crypto::AesCcm        aesCcm;
    Crypto::AesCcm::Nonce nonce;

    VerifyOrExit(GetSecurityEnabled());

    SuccessOrExit(GetSecurityLevel(securityLevel));
    SuccessOrExit(GetFrameCounter(frameCounter));

    nonce.InitFrom(aExtAddress, frameCounter, securityLevel);

    aesCcm.SetKey(GetAesKey());
    aesCcm.SetNonce(nonce);
    aesCcm.SetAuthData(GetHeader(), GetHeaderLength());
    aesCcm.SetTagLength(GetFooterLength() - GetFcsSize());

    SuccessOrExit(aesCcm.Process(Crypto::AesCcm::kEncrypt, GetPayload(), GetPayloadLength()));

    SetIsSecurityProcessed(true);

exit:
    return;
#else
    OT_UNUSED_VARIABLE(aExtAddress);
#endif // OPENTHREAD_FTD || OPENTHREAD_MTD || OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE
}

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT && OPENTHREAD_CONFIG_MAC_SOFTWARE_RETX_SECURITY_ENABLE
void TxFrame::DecryptTransmitAesCcm(const ExtAddress &aExtAddress)
{
    uint32_t              frameCounter = 0;
    uint8_t               securityLevel;
    Crypto::AesCcm        aesCcm;
    Crypto::AesCcm::Nonce nonce;

    VerifyOrExit(GetSecurityEnabled() && IsSecurityProcessed());

    SuccessOrExit(GetSecurityLevel(securityLevel));
    SuccessOrExit(GetFrameCounter(frameCounter));

    nonce.InitFrom(aExtAddress, frameCounter, securityLevel);

    aesCcm.SetKey(GetAesKey());
    aesCcm.SetNonce(nonce);
    aesCcm.SetAuthData(GetHeader(), GetHeaderLength());
    aesCcm.SetTagLength(GetFooterLength() - GetFcsSize());

    // We expect success because we are only decrypting back to plaintext,
    // and we know the ciphertext was generated correctly by us previously.
    IgnoreError(aesCcm.Process(Crypto::AesCcm::kDecrypt, GetPayload(), GetPayloadLength()));

    SetIsSecurityProcessed(false);
    SetIsHeaderUpdated(false);

exit:
    return;
}
#endif // OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT && OPENTHREAD_CONFIG_MAC_SOFTWARE_RETX_SECURITY_ENABLE

void TxFrame::GenerateImmAck(const RxFrame &aFrame, bool aIsFramePending)
{
    uint16_t fcf = static_cast<uint16_t>(kTypeAck) | aFrame.GetVersion();

    mChannel = aFrame.mChannel;
    ClearAllBytes(mInfo.mTxInfo);

    if (aIsFramePending)
    {
        fcf |= kFcfFramePending;
    }
    LittleEndian::WriteUint16(fcf, mPsdu);

    mPsdu[kFcfSize] = aFrame.GetSequence();

    mLength = kImmAckLength;
}

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
Error TxFrame::GenerateEnhAck(const RxFrame &aRxFrame, bool aIsFramePending, const uint8_t *aIeData, uint8_t aIeLength)
{
    Error     error = kErrorNone;
    BuildInfo buildInfo;
    Address   address;
    PanId     panId;
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

    SuccessOrExit(error = aRxFrame.GetSrcAddr(buildInfo.mAddrs.mDestination));
    VerifyOrExit(!buildInfo.mAddrs.mDestination.IsNone(), error = kErrorParse);

    if (aRxFrame.GetSecurityEnabled())
    {
        SuccessOrExit(error = aRxFrame.GetSecurityLevel(securityLevel));
        VerifyOrExit(securityLevel == kSecurityEncMic32, error = kErrorParse);

        SuccessOrExit(error = aRxFrame.GetKeyIdMode(keyIdMode));
    }

    if (aRxFrame.IsSrcPanIdPresent())
    {
        SuccessOrExit(error = aRxFrame.GetSrcPanId(panId));
        buildInfo.mPanIds.SetDestination(panId);
    }
    else if (aRxFrame.IsDstPanIdPresent())
    {
        SuccessOrExit(error = aRxFrame.GetDstPanId(panId));
        buildInfo.mPanIds.SetDestination(panId);
    }

    // Prepare the ack frame

    mChannel = aRxFrame.mChannel;
    ClearAllBytes(mInfo.mTxInfo);

    buildInfo.mType          = kTypeAck;
    buildInfo.mVersion       = kVersion2015;
    buildInfo.mSecurityLevel = static_cast<SecurityLevel>(securityLevel);
    buildInfo.mKeyIdMode     = static_cast<KeyIdMode>(keyIdMode);

    buildInfo.PrepareHeadersIn(*this);

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

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
Error TxFrame::GenerateWakeupFrame(PanId aPanId, const WakeupRequest &aWakeupRequest, const Address &aSource)
{
    // Placeholder implementation following removal of legacy Multipurpose frame format.
    OT_UNUSED_VARIABLE(aPanId);
    OT_UNUSED_VARIABLE(aWakeupRequest);
    OT_UNUSED_VARIABLE(aSource);

    return kErrorFailed;
}
#endif

bool RxFrame::IsSecuredWith(KeyIdModeFlags aFlags) const
{
    bool    isSecure = false;
    uint8_t keyIdMode;

    VerifyOrExit(GetSecurityEnabled());
    SuccessOrExit(GetKeyIdMode(keyIdMode));

    switch (keyIdMode)
    {
    case kKeyIdMode0:
        VerifyOrExit(aFlags & kAllowKeyIdMode0);
        break;
    case kKeyIdMode1:
        VerifyOrExit(aFlags & kAllowKeyIdMode1);
        break;
    default:
        ExitNow();
    }

    isSecure = true;

exit:
    return isSecure;
}

Error RxFrame::ProcessReceiveAesCcm(const ExtAddress &aExtAddress, const KeyMaterial &aMacKey)
{
#if OPENTHREAD_FTD || OPENTHREAD_MTD
    Error                 error        = kErrorSecurity;
    uint32_t              frameCounter = 0;
    uint8_t               securityLevel;
    Crypto::AesCcm        aesCcm;
    Crypto::AesCcm::Nonce nonce;

    VerifyOrExit(GetSecurityEnabled(), error = kErrorNone);

    SuccessOrExit(GetSecurityLevel(securityLevel));
    SuccessOrExit(GetFrameCounter(frameCounter));

    nonce.InitFrom(aExtAddress, frameCounter, securityLevel);

    aesCcm.SetKey(aMacKey);
    aesCcm.SetNonce(nonce);
    aesCcm.SetAuthData(GetHeader(), GetHeaderLength());
    aesCcm.SetTagLength(GetFooterLength() - GetFcsSize());

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    // Do not decrypt when fuzzing
    ExitNow(error = kErrorNone);
#endif

    error = aesCcm.Process(Crypto::AesCcm::kDecrypt, GetPayload(), GetPayloadLength());

exit:
    return error;
#else
    OT_UNUSED_VARIABLE(aExtAddress);
    OT_UNUSED_VARIABLE(aMacKey);

    return kErrorNone;
#endif // OPENTHREAD_FTD || OPENTHREAD_MTD
}

// LCOV_EXCL_START

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE)

Frame::InfoString Frame::ToInfoString(void) const
{
    InfoString string;
    uint8_t    commandId, type;
    Address    src, dst;
    uint32_t   frameCounter;
    bool       sequencePresent;

    string.Append("len:%d", mLength);

    sequencePresent = IsSequencePresent();

    if (sequencePresent)
    {
        string.Append(", seqnum:%d", GetSequence());
    }

    string.Append(", type:");

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

    if (!sequencePresent && GetFrameCounter(frameCounter) == kErrorNone)
    {
        string.Append(", fc:%lu", ToUlong(frameCounter));
    }

#if OPENTHREAD_CONFIG_MULTI_RADIO
    string.Append(", radio:%s", RadioTypeToString(GetRadioType()));
#endif

    return string;
}

#endif // #if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE)

// LCOV_EXCL_STOP

} // namespace Mac
} // namespace ot
