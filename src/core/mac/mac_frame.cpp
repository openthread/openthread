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
#if !OPENTHREAD_RADIO || OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE
#include "crypto/aes_ccm.hpp"
#endif

namespace ot {
namespace Mac {

using ot::Encoding::LittleEndian::ReadUint16;
using ot::Encoding::LittleEndian::ReadUint32;
using ot::Encoding::LittleEndian::WriteUint16;
using ot::Encoding::LittleEndian::WriteUint32;

void Frame::InitMacHeader(uint16_t aFcf, uint8_t aSecurityControl)
{
    mLength = CalculateAddrFieldSize(aFcf);

    OT_ASSERT(mLength != kInvalidSize);

    WriteUint16(aFcf, mPsdu);

    if (aFcf & kFcfSecurityEnabled)
    {
        mPsdu[mLength] = aSecurityControl;

        mLength += CalculateSecurityHeaderSize(aSecurityControl);
        mLength += CalculateMicSize(aSecurityControl);
    }

    if ((aFcf & kFcfFrameTypeMask) == kFcfFrameMacCmd)
    {
        mLength += kCommandIdSize;
    }

    mLength += GetFcsSize();
}

uint16_t Frame::GetFrameControlField(void) const
{
    return ReadUint16(mPsdu);
}

otError Frame::ValidatePsdu(void) const
{
    otError error = OT_ERROR_NONE;
    uint8_t index = FindPayloadIndex();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);
    VerifyOrExit((index + GetFooterLength()) <= mLength, error = OT_ERROR_PARSE);

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

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    if (IsVersion2015(aFcf))
    {
        switch (aFcf & (kFcfDstAddrMask | kFcfSrcAddrMask | kFcfPanidCompression))
        {
        case (kFcfDstAddrNone | kFcfSrcAddrNone):
        case (kFcfDstAddrExt | kFcfSrcAddrNone | kFcfPanidCompression):
        case (kFcfDstAddrShort | kFcfSrcAddrNone | kFcfPanidCompression):
        case (kFcfDstAddrNone | kFcfSrcAddrExt):
        case (kFcfDstAddrNone | kFcfSrcAddrShort):
        case (kFcfDstAddrNone | kFcfSrcAddrExt | kFcfPanidCompression):
        case (kFcfDstAddrNone | kFcfSrcAddrShort | kFcfPanidCompression):
        case (kFcfDstAddrExt | kFcfSrcAddrExt | kFcfPanidCompression):
            present = false;
            break;
        default:
            break;
        }
    }
    else
#endif
    {
        present = IsDstAddrPresent(aFcf);
    }

    return present;
}

otError Frame::GetDstPanId(PanId &aPanId) const
{
    otError error = OT_ERROR_NONE;
    uint8_t index = FindDstPanIdIndex();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);
    aPanId = ReadUint16(&mPsdu[index]);

exit:
    return error;
}

void Frame::SetDstPanId(PanId aPanId)
{
    uint8_t index = FindDstPanIdIndex();

    OT_ASSERT(index != kInvalidIndex);
    WriteUint16(aPanId, &mPsdu[index]);
}

uint8_t Frame::FindDstAddrIndex(void) const
{
    return kFcfSize + kDsnSize + (IsDstPanIdPresent() ? sizeof(PanId) : 0);
}

otError Frame::GetDstAddr(Address &aAddress) const
{
    otError error = OT_ERROR_NONE;
    uint8_t index = FindDstAddrIndex();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);

    switch (GetFrameControlField() & kFcfDstAddrMask)
    {
    case kFcfDstAddrShort:
        aAddress.SetShort(ReadUint16(&mPsdu[index]));
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
    WriteUint16(aShortAddress, &mPsdu[FindDstAddrIndex()]);
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
    bool srcPanIdPresent = false;

    if ((aFcf & kFcfSrcAddrMask) != kFcfSrcAddrNone && (aFcf & kFcfPanidCompression) == 0)
    {
#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
        // Handle a special case in IEEE 802.15.4-2015, when Pan ID Compression is 0, but Src Pan ID is not present:
        //  Dest Address:       Extended
        //  Source Address:     Extended
        //  Dest Pan ID:        Present
        //  Src Pan ID:         Not Present
        //  Pan ID Compression: 0
        if (!IsVersion2015(aFcf) || (aFcf & kFcfDstAddrMask) != kFcfDstAddrExt ||
            (aFcf & kFcfSrcAddrMask) != kFcfSrcAddrExt)
#endif
        {
            srcPanIdPresent = true;
        }
    }

    return srcPanIdPresent;
}

otError Frame::GetSrcPanId(PanId &aPanId) const
{
    otError error = OT_ERROR_NONE;
    uint8_t index = FindSrcPanIdIndex();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);
    aPanId = ReadUint16(&mPsdu[index]);

exit:
    return error;
}

otError Frame::SetSrcPanId(PanId aPanId)
{
    otError error = OT_ERROR_NONE;
    uint8_t index = FindSrcPanIdIndex();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);
    WriteUint16(aPanId, &mPsdu[index]);

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

otError Frame::GetSrcAddr(Address &aAddress) const
{
    otError  error = OT_ERROR_NONE;
    uint8_t  index = FindSrcAddrIndex();
    uint16_t fcf   = GetFrameControlField();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);

    switch (fcf & kFcfSrcAddrMask)
    {
    case kFcfSrcAddrShort:
        aAddress.SetShort(ReadUint16(&mPsdu[index]));
        break;

    case kFcfSrcAddrExt:
        aAddress.SetExtended(&mPsdu[index], ExtAddress::kReverseByteOrder);
        break;

    default:
        aAddress.SetNone();
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

    WriteUint16(aShortAddress, &mPsdu[index]);
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

otError Frame::GetSecurityControlField(uint8_t &aSecurityControlField) const
{
    otError error = OT_ERROR_NONE;
    uint8_t index = FindSecurityHeaderIndex();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);

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

otError Frame::GetSecurityLevel(uint8_t &aSecurityLevel) const
{
    otError error = OT_ERROR_NONE;
    uint8_t index = FindSecurityHeaderIndex();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);

    aSecurityLevel = mPsdu[index] & kSecLevelMask;

exit:
    return error;
}

otError Frame::GetKeyIdMode(uint8_t &aKeyIdMode) const
{
    otError error = OT_ERROR_NONE;
    uint8_t index = FindSecurityHeaderIndex();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);

    aKeyIdMode = mPsdu[index] & kKeyIdModeMask;

exit:
    return error;
}

otError Frame::GetFrameCounter(uint32_t &aFrameCounter) const
{
    otError error = OT_ERROR_NONE;
    uint8_t index = FindSecurityHeaderIndex();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);

    // Security Control
    index += kSecurityControlSize;

    aFrameCounter = ReadUint32(&mPsdu[index]);

exit:
    return error;
}

void Frame::SetFrameCounter(uint32_t aFrameCounter)
{
    uint8_t index = FindSecurityHeaderIndex();

    OT_ASSERT(index != kInvalidIndex);

    // Security Control
    index += kSecurityControlSize;

    WriteUint32(aFrameCounter, &mPsdu[index]);
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

otError Frame::GetKeyId(uint8_t &aKeyId) const
{
    otError error = OT_ERROR_NONE;
    uint8_t keySourceLength;
    uint8_t index = FindSecurityHeaderIndex();

    VerifyOrExit(index != kInvalidIndex, OT_NOOP);

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

otError Frame::GetCommandId(uint8_t &aCommandId) const
{
    otError error = OT_ERROR_NONE;
    uint8_t index = FindPayloadIndex();
    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);

    aCommandId = mPsdu[IsVersion2015() ? index : (index - 1)];

exit:
    return error;
}

otError Frame::SetCommandId(uint8_t aCommandId)
{
    otError error = OT_ERROR_NONE;
    uint8_t index = FindPayloadIndex();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);

    mPsdu[IsVersion2015() ? index : (index - 1)] = aCommandId;

exit:
    return error;
}

bool Frame::IsDataRequestCommand(void) const
{
    bool    isDataRequest = false;
    uint8_t commandId;

    VerifyOrExit(GetType() == kFcfFrameMacCmd, OT_NOOP);
    SuccessOrExit(GetCommandId(commandId));
    isDataRequest = (commandId == kMacCmdDataRequest);

exit:
    return isDataRequest;
}

uint8_t Frame::GetHeaderLength(void) const
{
    return static_cast<uint8_t>(GetPayload() - mPsdu);
}

uint8_t Frame::GetFooterLength(void) const
{
    uint8_t footerLength = static_cast<uint8_t>(GetFcsSize());
    uint8_t index        = FindSecurityHeaderIndex();

    VerifyOrExit(index != kInvalidIndex, OT_NOOP);
    footerLength += CalculateMicSize(mPsdu[index]);

exit:
    return footerLength;
}

uint8_t Frame::CalculateMicSize(uint8_t aSecurityControl)
{
    uint8_t micSize = 0;

    switch (aSecurityControl & kSecLevelMask)
    {
    case kSecNone:
    case kSecEnc:
        micSize = kMic0Size;
        break;

    case kSecMic32:
    case kSecEncMic32:
        micSize = kMic32Size;
        break;

    case kSecMic64:
    case kSecEncMic64:
        micSize = kMic64Size;
        break;

    case kSecMic128:
    case kSecEncMic128:
        micSize = kMic128Size;
        break;
    }

    return micSize;
}

uint16_t Frame::GetMaxPayloadLength(void) const
{
    return GetMtu() - (GetHeaderLength() + GetFooterLength());
}

uint16_t Frame::GetPayloadLength(void) const
{
    return mLength - (GetHeaderLength() + GetFooterLength());
}

void Frame::SetPayloadLength(uint16_t aLength)
{
    mLength = GetHeaderLength() + GetFooterLength() + aLength;
}

uint8_t Frame::SkipSecurityHeaderIndex(void) const
{
    uint8_t index = SkipAddrFieldIndex();

    VerifyOrExit(index != kInvalidIndex, OT_NOOP);

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

    VerifyOrExit((aSecurityControl & kSecLevelMask) != kSecNone, size = kInvalidSize);

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

    VerifyOrExit(index != kInvalidIndex, OT_NOOP);

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

            if (ie->GetId() == kHeaderIeTermination2)
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

    if (!IsVersion2015() && (GetFrameControlField() & kFcfFrameTypeMask) == kFcfFrameMacCmd)
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

const uint8_t *Frame::GetFooter(void) const
{
    return mPsdu + mLength - GetFooterLength();
}

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
uint8_t Frame::FindHeaderIeIndex(void) const
{
    uint8_t index;

    VerifyOrExit(IsIePresent(), index = kInvalidIndex);

    index = SkipSecurityHeaderIndex();

exit:
    return index;
}

otError Frame::AppendHeaderIe(HeaderIe *aIeList, uint8_t aIeCount)
{
    otError  error = OT_ERROR_NONE;
    uint16_t index = FindHeaderIeIndex();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_NOT_FOUND);

    for (uint8_t i = 0; i < aIeCount; i++)
    {
        memcpy(&mPsdu[index], &aIeList[i], sizeof(HeaderIe));

        index += sizeof(HeaderIe) + aIeList[i].GetLength();
        mLength += sizeof(HeaderIe) + aIeList[i].GetLength();
    }

exit:
    return error;
}

const uint8_t *Frame::GetHeaderIe(uint8_t aIeId) const
{
    uint8_t        index        = FindHeaderIeIndex();
    uint8_t        payloadIndex = FindPayloadIndex();
    const uint8_t *header       = nullptr;

    // `FindPayloadIndex()` verifies that Header IE(s) in frame (if present)
    // are well-formed.

    VerifyOrExit((index != kInvalidIndex) && (payloadIndex != kInvalidIndex), OT_NOOP);

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
#endif // OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
void Frame::SetCslIe(uint16_t aCslPeriod, uint16_t aCslPhase)
{
    uint8_t *cur = GetHeaderIe(Frame::kHeaderIeCsl);
    CslIe *  csl;

    VerifyOrExit(cur != nullptr, OT_NOOP);

    csl = reinterpret_cast<CslIe *>(cur + sizeof(HeaderIe));
    csl->SetPeriod(aCslPeriod);
    csl->SetPhase(aCslPhase);
exit:
    return;
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
const TimeIe *Frame::GetTimeIe(void) const
{
    const TimeIe * timeIe = nullptr;
    const uint8_t *cur    = nullptr;

    cur = GetHeaderIe(kHeaderIeVendor);
    VerifyOrExit(cur != nullptr, OT_NOOP);

    cur += sizeof(HeaderIe);

    timeIe = reinterpret_cast<const TimeIe *>(cur);
    VerifyOrExit(timeIe->GetVendorOui() == TimeIe::kVendorOuiNest, timeIe = nullptr);
    VerifyOrExit(timeIe->GetSubType() == TimeIe::kVendorIeTime, timeIe = nullptr);

exit:
    return timeIe;
}
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

void TxFrame::CopyFrom(const TxFrame &aFromFrame)
{
    uint8_t *      psduBuffer   = mPsdu;
    otRadioIeInfo *ieInfoBuffer = mInfo.mTxInfo.mIeInfo;

    memcpy(this, &aFromFrame, sizeof(Frame));

    // Set the original buffer pointers back on the frame
    // which were overwritten by above `memcpy()`.

    mPsdu                 = psduBuffer;
    mInfo.mTxInfo.mIeInfo = ieInfoBuffer;

    memcpy(mPsdu, aFromFrame.mPsdu, aFromFrame.mLength);

    // mIeInfo may be null when TIME_SYNC is not enabled.
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    memcpy(mInfo.mTxInfo.mIeInfo, aFromFrame.mInfo.mTxInfo.mIeInfo, sizeof(otRadioIeInfo));
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

    VerifyOrExit(GetSecurityEnabled(), OT_NOOP);

    SuccessOrExit(GetSecurityLevel(securityLevel));
    SuccessOrExit(GetFrameCounter(frameCounter));

    Crypto::AesCcm::GenerateNonce(aExtAddress, frameCounter, securityLevel, nonce);

    aesCcm.SetKey(GetAesKey());
    tagLength = GetFooterLength() - Frame::kFcsSize;

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
    uint16_t fcf = kFcfFrameAck | aFrame.GetVersion();

    mChannel = aFrame.mChannel;
    memset(&mInfo.mTxInfo, 0, sizeof(mInfo.mTxInfo));

    if (aIsFramePending)
    {
        fcf |= kFcfFramePending;
    }
    WriteUint16(fcf, mPsdu);

    mPsdu[kSequenceIndex] = aFrame.GetSequence();

    mLength = kImmAckLength;
}

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
otError TxFrame::GenerateEnhAck(const RxFrame &aFrame, bool aIsFramePending, const uint8_t *aIeData, uint8_t aIeLength)
{
    otError error = OT_ERROR_NONE;

    uint16_t fcf = kFcfFrameAck | kFcfFrameVersion2015 | kFcfSrcAddrNone;
    Address  address;
    PanId    panId;
    uint8_t  footerLength;
    uint8_t  securityControlField;
    uint8_t  keyId;

    mChannel = aFrame.mChannel;
    memset(&mInfo.mTxInfo, 0, sizeof(mInfo.mTxInfo));

    // Set frame control field
    if (aIsFramePending)
    {
        fcf |= kFcfFramePending;
    }

    if (aFrame.GetSecurityEnabled())
    {
        fcf |= kFcfSecurityEnabled;
    }

    if (aFrame.IsPanIdCompressed())
    {
        fcf |= kFcfPanidCompression;
    }

    // Destination address mode
    if ((aFrame.GetFrameControlField() & kFcfSrcAddrMask) == kFcfSrcAddrExt)
    {
        fcf |= kFcfDstAddrExt;
    }
    else if ((aFrame.GetFrameControlField() & kFcfSrcAddrMask) == kFcfSrcAddrShort)
    {
        fcf |= kFcfDstAddrShort;
    }
    else
    {
        fcf |= kFcfDstAddrNone;
    }

    if (aIeLength > 0)
    {
        fcf |= kFcfIePresent;
    }

    WriteUint16(fcf, mPsdu);

    // Set sequence number
    mPsdu[kSequenceIndex] = aFrame.GetSequence();

    if (IsDstPanIdPresent())
    {
        // Set address field
        if (aFrame.IsSrcPanIdPresent())
        {
            SuccessOrExit(error = aFrame.GetSrcPanId(panId));
        }
        else if (aFrame.IsDstPanIdPresent())
        {
            SuccessOrExit(error = aFrame.GetDstPanId(panId));
        }
        else
        {
            ExitNow(error = OT_ERROR_PARSE);
        }

        SetDstPanId(panId);
    }

    if (aFrame.IsSrcAddrPresent())
    {
        SuccessOrExit(error = aFrame.GetSrcAddr(address));
        SetDstAddr(address);
    }

    // At this time the length of ACK hasn't been determined, set it to
    // `kMaxPsduSize` to call methods that check frame length
    mLength = kMaxPsduSize;

    // Set security header
    if (aFrame.GetSecurityEnabled())
    {
        SuccessOrExit(error = aFrame.GetSecurityControlField(securityControlField));
        SuccessOrExit(error = aFrame.GetKeyId(keyId));

        SetSecurityControlField(securityControlField);
        SetKeyId(keyId);
    }

    // Set header IE
    if (aIeLength > 0)
    {
        OT_ASSERT(aIeData != nullptr);
        memcpy(&mPsdu[FindHeaderIeIndex()], aIeData, aIeLength);
    }

    // Set frame length
    footerLength = GetFooterLength();
    OT_ASSERT(footerLength != kInvalidIndex);
    mLength = SkipSecurityHeaderIndex() + aIeLength + footerLength;

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2

otError RxFrame::ProcessReceiveAesCcm(const ExtAddress &aExtAddress, const Key &aMacKey)
{
#if OPENTHREAD_RADIO
    OT_UNUSED_VARIABLE(aExtAddress);
    OT_UNUSED_VARIABLE(aMacKey);

    return OT_ERROR_NONE;
#else
    otError error = OT_ERROR_SECURITY;
    uint32_t frameCounter = 0;
    uint8_t securityLevel;
    uint8_t nonce[Crypto::AesCcm::kNonceSize];
    uint8_t tag[kMaxMicSize];
    uint8_t tagLength;
    Crypto::AesCcm aesCcm;

    VerifyOrExit(GetSecurityEnabled(), error = OT_ERROR_NONE);

    SuccessOrExit(GetSecurityLevel(securityLevel));
    SuccessOrExit(GetFrameCounter(frameCounter));

    Crypto::AesCcm::GenerateNonce(aExtAddress, frameCounter, securityLevel, nonce);

    aesCcm.SetKey(aMacKey);
    tagLength = GetFooterLength() - Frame::kFcsSize;

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
    VerifyOrExit(memcmp(tag, GetFooter(), tagLength) == 0, OT_NOOP);
#endif

    error = OT_ERROR_NONE;

exit:
    return error;
#endif // OPENTHREAD_RADIO
}

// LCOV_EXCL_START

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_NOTE) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

Frame::InfoString Frame::ToInfoString(void) const
{
    InfoString string;
    uint8_t    commandId, type;
    Address    src, dst;

    IgnoreError(string.Append("len:%d, seqnum:%d, type:", mLength, GetSequence()));

    type = GetType();

    switch (type)
    {
    case kFcfFrameBeacon:
        IgnoreError(string.Append("Beacon"));
        break;

    case kFcfFrameData:
        IgnoreError(string.Append("Data"));
        break;

    case kFcfFrameAck:
        IgnoreError(string.Append("Ack"));
        break;

    case kFcfFrameMacCmd:
        if (GetCommandId(commandId) != OT_ERROR_NONE)
        {
            commandId = 0xff;
        }

        switch (commandId)
        {
        case kMacCmdDataRequest:
            IgnoreError(string.Append("Cmd(DataReq)"));
            break;

        case kMacCmdBeaconRequest:
            IgnoreError(string.Append("Cmd(BeaconReq)"));
            break;

        default:
            IgnoreError(string.Append("Cmd(%d)", commandId));
            break;
        }

        break;

    default:
        IgnoreError(string.Append("%d", type));
        break;
    }

    IgnoreError(GetSrcAddr(src));
    IgnoreError(GetDstAddr(dst));

    IgnoreError(string.Append(", src:%s, dst:%s, sec:%s, ackreq:%s", src.ToString().AsCString(),
                              dst.ToString().AsCString(), GetSecurityEnabled() ? "yes" : "no",
                              GetAckRequest() ? "yes" : "no"));

    return string;
}

BeaconPayload::InfoString BeaconPayload::ToInfoString(void) const
{
    NetworkName name;

    IgnoreError(name.Set(GetNetworkName()));

    return InfoString("name:%s, xpanid:%s, id:%d, ver:%d, joinable:%s, native:%s", name.GetAsCString(),
                      mExtendedPanId.ToString().AsCString(), GetProtocolId(), GetProtocolVersion(),
                      IsJoiningPermitted() ? "yes" : "no", IsNative() ? "yes" : "no");
}

#endif // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_NOTE) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

// LCOV_EXCL_STOP

} // namespace Mac
} // namespace ot
