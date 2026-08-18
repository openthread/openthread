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
#include "common/log.hpp"
#include "common/num_utils.hpp"
#include "crypto/aes_ccm.hpp"

namespace ot {
namespace Mac {

//----------------------------------------------------------------------------------------------------------------------

void TxFrame::BuildInfo::PrepareHeadersIn(TxFrame &aTxFrame) const
{
    uint16_t     fcf;
    FrameBuilder builder;
    uint8_t      micSize = 0;

    fcf = static_cast<uint16_t>(mType) | static_cast<uint16_t>(mVersion);

    fcf |= static_cast<uint16_t>(DetermineAddrMode(mAddrs.mSource) << kFcfSrcAddrShift);
    fcf |= static_cast<uint16_t>(DetermineAddrMode(mAddrs.mDestination) << kFcfDstAddrShift);

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
        fcf |= kFcfSeqSuppression;
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

    if (IsSeqPresent(fcf))
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
        uint8_t secCtl = ConstructSecurityControlField(mSecurityLevel, mKeyIdMode);
        uint8_t size =
            kFrameCounterSize + CalculateKeySourceSize(mKeyIdMode) + ((mKeyIdMode != kKeyIdMode0) ? kKeyIndexSize : 0);

        IgnoreError(builder.AppendUint8(secCtl));
        builder.AppendLength(size);

        micSize = CalculateMicSize(mSecurityLevel);
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

//----------------------------------------------------------------------------------------------------------------------

Error Frame::ParseInfo::ParseFrom(const Frame &aFrame, ParseMode aMode)
{
    // Parses and validates the MAC frame header and extracts header
    // fields according to `aMode`:
    //
    // - `kParseAddrFields`: Parses up through address fields
    //   (FCF, sequence number, PAN IDs, source and destination
    //   addresses) and FCS.
    //
    // - `kParseSecurityHeader`: Parses up through Auxiliary Security
    //   Header. Under `kParseSecurityHeader` mode, the frame is
    //   explicitly required to have a security header (security enabled
    //   in FCF). Otherwise `kErrorNotFound` is returned.
    //
    // - `kParseFully`: Parses all header fields including Auxiliary
    //   Security Header, Header IEs, and MAC Command ID (if
    //   applicable), determining the exact header and payload
    //   boundaries (`mHeader` and `mPayload`).

    Error     error = kErrorParse;
    FrameData frameData;
    uint16_t  value;
    uint8_t   size;

    VerifyOrExit(aFrame.GetPsdu() != nullptr);

    frameData.Init(aFrame.GetPsdu(), aFrame.GetLength());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Address Fields

    SuccessOrExit(frameData.ReadUint<kLittleEndian>(mFcf));

    // Only accept standard frame types (Beacon, Data, Ack, MAC Command).
    // Other types (e.g., Multipurpose) use a different FCF/header layout.
    // Also restrict frame version to 2003, 2006, 2015. Future frame
    // versions can alter the MAC header layout.

    VerifyOrExit(GetType(mFcf) <= kTypeMacCmd);
    VerifyOrExit(GetVersion(mFcf) <= kVersion2015);

    if (IsSeqPresent(mFcf))
    {
        SuccessOrExit(frameData.ReadUint8(mSequenceNum));
    }

    if (IsDstPanIdPresent(mFcf))
    {
        SuccessOrExit(frameData.ReadUint<kLittleEndian>(value));
        mPanIds.SetDestination(value);
    }

    SuccessOrExit(ParseAddress(frameData, ReadDstAddrMode(mFcf), mAddrs.mDestination));

    if (IsSrcPanIdPresent(mFcf))
    {
        SuccessOrExit(frameData.ReadUint<kLittleEndian>(value));
        mPanIds.SetSource(value);
    }

    SuccessOrExit(ParseAddress(frameData, ReadSrcAddrMode(mFcf), mAddrs.mSource));

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - -
    // FCS

    SuccessOrExit(frameData.RemoveFooter(aFrame.GetFcsSize()));

    if (aMode == kParseAddrFields)
    {
        ExitNow(error = kErrorNone);
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Aux Security Header

    if (IsSecurityEnabled(mFcf))
    {
        SuccessOrExit(frameData.ReadUint8(mSecCtl));

        mSecurityLevel = ReadSecurityLevel(mSecCtl);
        mKeyIdMode     = ReadKeyIdMode(mSecCtl);

        VerifyOrExit(mSecurityLevel != kSecurityNone);

        mFrameCounterBytes = AsNonConst(frameData.GetBytes());
        SuccessOrExit(frameData.ReadUint<kLittleEndian>(mFrameCounter));

        size = CalculateKeySourceSize(mKeyIdMode);

        VerifyOrExit(frameData.CanRead(size));
        mKeySource.Init(frameData.GetBytes(), size);
        frameData.SkipOver(size);

        if (mKeyIdMode != kKeyIdMode0)
        {
            mKeyIndexByte = AsNonConst(frameData.GetBytes());
            SuccessOrExit(frameData.ReadUint8(mKeyIndex));
        }

        mMicSize = CalculateMicSize(mSecurityLevel);
        SuccessOrExit(frameData.RemoveFooter(mMicSize));
    }

    if (aMode == kParseSecurityHeader)
    {
        VerifyOrExit(IsSecurityEnabled(mFcf), error = kErrorNotFound);
        ExitNow(error = kErrorNone);
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Header IE

    if (IsIePresent(mFcf))
    {
#if !OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
        ExitNow();
#else

        mIeData = frameData;

        do
        {
            const HeaderIe *ie = frameData.Read<HeaderIe>();

            VerifyOrExit(ie != nullptr);

            VerifyOrExit(frameData.CanRead(ie->GetLength()));
            frameData.SkipOver(ie->GetLength());

            if (ie->GetId() == Termination2Ie::kId)
            {
                break;
            }

            // If the `frameData.IsEmpty()`, we exit the `while()`
            // loop. This covers the case where frame contains one or more
            // Header IEs but no data payload. In this case, spec does not
            // require Header IE termination to be included (it is optional)
            // since the end of frame can be determined from frame length and
            // footer length.

        } while (!frameData.IsEmpty());

        mIeData.InitFromRange(mIeData.GetBytes(), frameData.GetBytes());

#endif // OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - - - - - - - - -
    // MAC Command

    if (GetType(mFcf) == kTypeMacCmd)
    {
        VerifyOrExit(frameData.CanRead(sizeof(mCommandId)));

        mCommandId = *frameData.GetBytes();

        // The treatment of the Command ID field in a MAC command frame
        // is version-dependent. In the 2015 spec, it is part of the
        // encrypted payload, while in earlier versions, it is part of
        // the MAC header.

        if (!IsVersion2015(mFcf))
        {
            frameData.SkipOver(sizeof(mCommandId));
        }
    }

    mHeader.InitFromRange(aFrame.GetPsdu(), frameData.GetBytes());
    mPayload = frameData;

    error = kErrorNone;

exit:
    return error;
}

Error Frame::ParseInfo::ParseAddress(FrameData &aFrameData, AddrMode aAddrMode, Address &aAddress)
{
    Error    error = kErrorNone;
    uint16_t shortAddr;

    switch (aAddrMode)
    {
    case kAddrModeNone:
        aAddress.SetNone();
        break;

    case kAddrModeShort:
        SuccessOrExit(error = aFrameData.ReadUint<kLittleEndian>(shortAddr));
        aAddress.SetShort(shortAddr);
        break;

    case kAddrModeExt:
        VerifyOrExit(aFrameData.CanRead(sizeof(ExtAddress)), error = kErrorParse);
        aAddress.SetExtended(aFrameData.GetBytes(), ExtAddress::kReverseByteOrder);
        aFrameData.SkipOver(sizeof(ExtAddress));
        break;

    default:
        error = kErrorParse;
        break;
    }

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------

Error Frame::ValidatePsdu(void) const
{
    ParseInfo info;

    return info.ParseFrom(*this, kParseFully);
}

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

    LittleEndian::WriteUint16(fcf, mPsdu);
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
    Error     error;
    ParseInfo info;

    SuccessOrExit(error = info.ParseFrom(*this, kParseAddrFields));
    VerifyOrExit(info.mPanIds.IsDestinationPresent(), error = kErrorNotFound);
    aPanId = info.mPanIds.GetDestination();

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

Error Frame::GetDstAddr(Address &aAddress) const
{
    Error     error;
    ParseInfo info;

    SuccessOrExit(error = info.ParseFrom(*this, kParseAddrFields));
    aAddress = info.mAddrs.mDestination;

exit:
    return error;
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
    Error     error;
    ParseInfo info;

    SuccessOrExit(error = info.ParseFrom(*this, kParseAddrFields));
    VerifyOrExit(info.mPanIds.IsSourcePresent(), error = kErrorNotFound);
    aPanId = info.mPanIds.GetSource();

exit:
    return error;
}

Error Frame::GetSrcAddr(Address &aAddress) const
{
    Error     error;
    ParseInfo info;

    SuccessOrExit(error = info.ParseFrom(*this, kParseAddrFields));
    aAddress = info.mAddrs.mSource;

exit:
    return error;
}

Error Frame::GetSecurityControlField(uint8_t &aSecurityControlField) const
{
    Error     error;
    ParseInfo info;

    SuccessOrExit(error = info.ParseFrom(*this, kParseSecurityHeader));
    aSecurityControlField = info.mSecCtl;

exit:
    return error;
}

Error Frame::GetSecurityLevel(SecurityLevel &aSecurityLevel) const
{
    Error     error;
    ParseInfo info;

    SuccessOrExit(error = info.ParseFrom(*this, kParseSecurityHeader));
    aSecurityLevel = info.mSecurityLevel;

exit:
    return error;
}

bool Frame::HasSecurityLevel(SecurityLevel aSecurityLevel) const
{
    bool          has = false;
    SecurityLevel securityLevel;

    SuccessOrExit(GetSecurityLevel(securityLevel));
    has = (securityLevel == aSecurityLevel);

exit:
    return has;
}

Error Frame::GetKeyIdMode(KeyIdMode &aKeyIdMode) const
{
    Error     error;
    ParseInfo info;

    SuccessOrExit(error = info.ParseFrom(*this, kParseSecurityHeader));
    aKeyIdMode = info.mKeyIdMode;

exit:
    return error;
}

bool Frame::HasKeyIdMode(KeyIdMode aKeyIdMode) const
{
    bool      has = false;
    KeyIdMode keyIdMode;

    SuccessOrExit(GetKeyIdMode(keyIdMode));
    has = (keyIdMode == aKeyIdMode);

exit:
    return has;
}

Error Frame::GetFrameCounter(uint32_t &aFrameCounter) const
{
    Error     error;
    ParseInfo info;

    SuccessOrExit(error = info.ParseFrom(*this, kParseSecurityHeader));
    aFrameCounter = info.mFrameCounter;

exit:
    return error;
}

void Frame::SetFrameCounter(uint32_t aFrameCounter)
{
    ParseInfo info;

    SuccessOrAssert(info.ParseFrom(*this, kParseSecurityHeader));
    LittleEndian::WriteUint32(aFrameCounter, info.mFrameCounterBytes);

    static_cast<TxFrame *>(this)->SetIsHeaderUpdated(true);
}

void Frame::GetKeySource(FrameData &aKeySource) const
{
    ParseInfo info;

    SuccessOrAssert(info.ParseFrom(*this, kParseSecurityHeader));
    aKeySource = info.mKeySource;
}

uint8_t Frame::CalculateKeySourceSize(KeyIdMode aKeyIdMode)
{
    static constexpr uint8_t kKeySourceSize[] = {
        /* [0] kKeyIdMode0 */ kKeySourceSizeMode0,
        /* [1] kKeyIdMode1 */ kKeySourceSizeMode1,
        /* [2] kKeyIdMode2 */ kKeySourceSizeMode2,
        /* [3] kKeyIdMode3 */ kKeySourceSizeMode3,
    };

    static_assert(kKeySourceSize[kKeyIdMode0] == kKeySourceSizeMode0, "kKeySourceSize[] array is incorrect");
    static_assert(kKeySourceSize[kKeyIdMode1] == kKeySourceSizeMode1, "kKeySourceSize[] array is incorrect");
    static_assert(kKeySourceSize[kKeyIdMode2] == kKeySourceSizeMode2, "kKeySourceSize[] array is incorrect");
    static_assert(kKeySourceSize[kKeyIdMode3] == kKeySourceSizeMode3, "kKeySourceSize[] array is incorrect");

    return kKeySourceSize[aKeyIdMode];
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void Frame::SetKeySource(const uint8_t *aKeySource)
{
    ParseInfo info;

    SuccessOrAssert(info.ParseFrom(*this, kParseSecurityHeader));
    memcpy(AsNonConst(info.mKeySource.GetBytes()), aKeySource, info.mKeySource.GetLength());
}

Error Frame::GetKeyIndex(uint8_t &aKeyIndex) const
{
    Error     error;
    ParseInfo info;

    SuccessOrExit(error = info.ParseFrom(*this, kParseSecurityHeader));
    VerifyOrExit(info.mKeyIdMode != kKeyIdMode0, error = kErrorNotFound);
    aKeyIndex = info.mKeyIndex;

exit:
    return error;
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void Frame::SetKeyIndex(uint8_t aKeyIndex)
{
    ParseInfo info;

    SuccessOrAssert(info.ParseFrom(*this, kParseSecurityHeader));
    VerifyOrExit(info.mKeyIdMode != kKeyIdMode0);
    *info.mKeyIndexByte = aKeyIndex;

exit:
    return;
}

Error Frame::GetCommandId(uint8_t &aCommandId) const
{
    Error     error;
    ParseInfo info;

    SuccessOrExit(error = info.ParseFrom(*this, kParseFully));
    VerifyOrExit(GetType(info.mFcf) == kTypeMacCmd, error = kErrorNotFound);
    aCommandId = info.mCommandId;

exit:
    return error;
}

bool Frame::IsDataRequestCommand(void) const
{
    bool    isDataRequest = false;
    uint8_t commandId;

    SuccessOrExit(GetCommandId(commandId));
    isDataRequest = (commandId == kMacCmdDataRequest);

exit:
    return isDataRequest;
}

Error Frame::GetPayload(FrameData &aPayloadData) const
{
    Error     error;
    ParseInfo info;

    SuccessOrExit(error = info.ParseFrom(*this, kParseFully));
    aPayloadData = info.mPayload;

exit:
    return error;
}

Error Frame::DetermineLengths(Lengths &aLengths) const
{
    Error     error;
    ParseInfo info;

    ClearAllBytes(aLengths);

    SuccessOrExit(error = info.ParseFrom(*this, kParseFully));

    aLengths.mHeader     = info.mHeader.GetLength();
    aLengths.mPayload    = info.mPayload.GetLength();
    aLengths.mFooter     = GetLength() - aLengths.mHeader - aLengths.mPayload;
    aLengths.mMaxPayload = GetMtu() - (aLengths.mHeader + aLengths.mFooter);

exit:
    return error;
}

uint8_t Frame::CalculateMicSize(SecurityLevel aSecurityLevel)
{
    static constexpr uint8_t kMicSize[] = {
        /* [0] kSecurityNone      */ kMic0Size,
        /* [1] kSecurityMic32     */ kMic32Size,
        /* [2] kSecurityMic64     */ kMic64Size,
        /* [3] kSecurityMic128    */ kMic128Size,
        /* [4] kSecurityEnc       */ kMic0Size,
        /* [5] kSecurityEncMic32  */ kMic32Size,
        /* [6] kSecurityEncMic64  */ kMic64Size,
        /* [7] kSecurityEncMic128 */ kMic128Size,
    };

    static_assert(kMicSize[kSecurityNone] == kMic0Size, "kMicSize[] array is incorrect");
    static_assert(kMicSize[kSecurityMic32] == kMic32Size, "kMicSize[] array is incorrect");
    static_assert(kMicSize[kSecurityMic64] == kMic64Size, "kMicSize[] array is incorrect");
    static_assert(kMicSize[kSecurityMic128] == kMic128Size, "kMicSize[] array is incorrect");
    static_assert(kMicSize[kSecurityEnc] == kMic0Size, "kMicSize[] array is incorrect");
    static_assert(kMicSize[kSecurityEncMic32] == kMic32Size, "kMicSize[] array is incorrect");
    static_assert(kMicSize[kSecurityEncMic64] == kMic64Size, "kMicSize[] array is incorrect");
    static_assert(kMicSize[kSecurityEncMic128] == kMic128Size, "kMicSize[] array is incorrect");

    return kMicSize[aSecurityLevel];
}

Frame::AddrMode Frame::DetermineAddrMode(const Address &aAddress)
{
    AddrMode addrMode = kAddrModeNone;

    switch (aAddress.GetType())
    {
    case Address::kTypeNone:
        break;
    case Address::kTypeShort:
        addrMode = kAddrModeShort;
        break;
    case Address::kTypeExtended:
        addrMode = kAddrModeExt;
        break;
    }

    return addrMode;
}

Frame::SecurityLevel Frame::ReadSecurityLevel(uint8_t aSecCtl)
{
    return static_cast<SecurityLevel>(ReadBits<uint8_t, kScfSecLevelMask>(aSecCtl));
}

Frame::KeyIdMode Frame::ReadKeyIdMode(uint8_t aSecCtl)
{
    return static_cast<KeyIdMode>(ReadBits<uint8_t, kScfKeyIdModeMask>(aSecCtl));
}

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT

const HeaderIe *Frame::FindHeaderIe(HeaderIeMatcher aMatcher) const
{
    const HeaderIe *matchedIe = nullptr;
    ParseInfo       info;

    SuccessOrExit(info.ParseFrom(*this, kParseFully));

    VerifyOrExit(IsIePresent(info.mFcf));

    // `ParseFrom()` already validates that Header IE(s) are
    // well-formed and contained within the frame. Here we
    // just iterate through them and try to match them.

    while (true)
    {
        const HeaderIe *ie = info.mIeData.Read<HeaderIe>();

        VerifyOrExit(ie != nullptr);

        if (aMatcher(*ie))
        {
            matchedIe = ie;
            ExitNow();
        }

        info.mIeData.SkipOver(ie->GetLength());
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

void TxFrame::PrepareHeaders(const BuildInfo &aBuildInfo, PayloadBuilder &aPayloadBuilder)
{
    aBuildInfo.PrepareHeadersIn(*this);
    aPayloadBuilder.InitFrom(*this);
}

void TxFrame::PayloadBuilder::InitFrom(TxFrame &aFrame)
{
    IgnoreError(aFrame.DetermineLengths(mLengths));
    Init(aFrame.GetPsduStartingAt(mLengths.mHeader), mLengths.mMaxPayload);
}

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
    VerifyOrExit(GetSecurityEnabled());
    SuccessOrExit(PerformAesCcm(kEncrypt, aExtAddress));
    SetIsSecurityProcessed(true);

exit:
    return;
#else
    OT_UNUSED_VARIABLE(aExtAddress);
#endif // OPENTHREAD_FTD || OPENTHREAD_MTD || OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE
}

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT && OPENTHREAD_CONFIG_MAC_SOFTWARE_RETX_SECURITY_ENABLE
void TxFrame::RestoreTransmitSecurity(const ExtAddress &aExtAddress)
{
    VerifyOrExit(GetSecurityEnabled() && IsSecurityProcessed());
    IgnoreError(PerformAesCcm(kDecrypt, aExtAddress));
    SetIsSecurityProcessed(false);

exit:
    SetIsHeaderUpdated(false);
}
#endif // OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT && OPENTHREAD_CONFIG_MAC_SOFTWARE_RETX_SECURITY_ENABLE

#if OPENTHREAD_FTD || OPENTHREAD_MTD || OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE || \
    (OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT && OPENTHREAD_CONFIG_MAC_SOFTWARE_RETX_SECURITY_ENABLE)

Error TxFrame::PerformAesCcm(AesCcmOperation aOperation, const ExtAddress &aExtAddress)
{
    static_assert(static_cast<uint8_t>(kEncrypt) == Crypto::AesCcm::kEncrypt, "kEncrypt enum value is incorrect");
    static_assert(static_cast<uint8_t>(kDecrypt) == Crypto::AesCcm::kDecrypt, "kDecrypt enum value is incorrect");

    Error                 error;
    ParseInfo             info;
    Crypto::AesCcm        aesCcm;
    Crypto::AesCcm::Nonce nonce;

    SuccessOrExit(error = info.ParseFrom(*this, kParseFully));

    nonce.InitFrom(aExtAddress, info.mFrameCounter, info.mSecurityLevel);

    aesCcm.SetKey(GetAesKey());
    aesCcm.SetNonce(nonce);
    aesCcm.SetAuthData(info.mHeader.GetBytes(), info.mHeader.GetLength());
    aesCcm.SetTagLength(info.mMicSize);

    error = aesCcm.Process(static_cast<Crypto::AesCcm::Operation>(aOperation), AsNonConst(info.mPayload.GetBytes()),
                           info.mPayload.GetLength());

exit:
    return error;
}

#endif // OPENTHREAD_FTD || OPENTHREAD_MTD || OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE || ...

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
    Error         error = kErrorNone;
    BuildInfo     buildInfo;
    Address       address;
    PanId         panId;
    SecurityLevel securityLevel = kSecurityNone;
    KeyIdMode     keyIdMode     = kKeyIdMode0;

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
        VerifyOrExit(aRxFrame.HasSecurityLevel(kSecurityEncMic32), error = kErrorParse);
        securityLevel = kSecurityEncMic32;

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
    buildInfo.mSecurityLevel = securityLevel;
    buildInfo.mKeyIdMode     = keyIdMode;

    PrepareHeadersWithEmptyPayload(buildInfo);

    SetFramePending(aIsFramePending);
    SetSequence(aRxFrame.GetSequence());

    if (aRxFrame.GetSecurityEnabled())
    {
        uint8_t keyIndex;

        SuccessOrExit(error = aRxFrame.GetKeyIndex(keyIndex));
        SetKeyIndex(keyIndex);
    }

    if (aIeLength > 0)
    {
        ParseInfo info;

        SuccessOrAssert(info.ParseFrom(*this, kParseFully));
        OT_ASSERT(aIeData != nullptr);

        SetIePresent(true);
        memcpy(GetPsduStartingAt(info.mHeader.GetLength()), aIeData, aIeLength);
        SetLength(GetLength() + aIeLength);
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
    bool      isSecure = false;
    KeyIdMode keyIdMode;

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

#if OPENTHREAD_FTD || OPENTHREAD_MTD

Error RxFrame::ProcessReceiveAesCcm(const ExtAddress &aExtAddress, const KeyMaterial &aMacKey)
{
    Error                 error = kErrorSecurity;
    ParseInfo             info;
    Crypto::AesCcm        aesCcm;
    Crypto::AesCcm::Nonce nonce;

    VerifyOrExit(GetSecurityEnabled(), error = kErrorNone);

    SuccessOrExit(info.ParseFrom(*this, kParseFully));

    nonce.InitFrom(aExtAddress, info.mFrameCounter, info.mSecurityLevel);

    aesCcm.SetKey(aMacKey);
    aesCcm.SetNonce(nonce);
    aesCcm.SetAuthData(info.mHeader.GetBytes(), info.mHeader.GetLength());
    aesCcm.SetTagLength(info.mMicSize);

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    // Do not decrypt when fuzzing
    ExitNow(error = kErrorNone);
#endif

    error = aesCcm.Process(Crypto::AesCcm::kDecrypt, AsNonConst(info.mPayload.GetBytes()), info.mPayload.GetLength());

exit:
    return error;
}

#endif // OPENTHREAD_FTD || OPENTHREAD_MTD

// LCOV_EXCL_START

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE)

Frame::InfoString Frame::ToInfoString(void) const
{
    InfoString string;
    ParseInfo  info;
    uint16_t   type;

    string.Append("len:%u", mLength);

    SuccessOrExit(info.ParseFrom(*this, kParseFully));

    if (IsSeqPresent(info.mFcf))
    {
        string.Append(", seqnum:%u", info.mSequenceNum);
    }

    string.Append(", type:");

    type = GetType(info.mFcf);

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
        switch (info.mCommandId)
        {
        case kMacCmdDataRequest:
            string.Append("Cmd(DataReq)");
            break;

        case kMacCmdBeaconRequest:
            string.Append("Cmd(BeaconReq)");
            break;

        default:
            string.Append("Cmd(%u)", info.mCommandId);
            break;
        }

        break;

    default:
        string.Append("%u", type);
        break;
    }

    string.Append(", src:%s, dst:%s, sec:%s, ackreq:%s", info.mAddrs.mSource.ToString().AsCString(),
                  info.mAddrs.mDestination.ToString().AsCString(), ToYesNo(IsSecurityEnabled(info.mFcf)),
                  ToYesNo(IsAckRequest(info.mFcf)));

    if (IsSecurityEnabled(info.mFcf))
    {
        string.Append(", fc:%lu", ToUlong(info.mFrameCounter));
    }

exit:
#if OPENTHREAD_CONFIG_MULTI_RADIO
    string.Append(", radio:%s", Radio::TypeToString(GetRadioType()));
#endif

    return string;
}

#endif // #if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE)

// LCOV_EXCL_STOP

} // namespace Mac
} // namespace ot
