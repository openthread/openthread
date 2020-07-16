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

void Frame::InitMacHeader(uint16_t aFcf, uint8_t aSecurityControl)
{
    uint8_t *bytes = GetPsdu();
    uint8_t  length;

    // Frame Control Field
    Encoding::LittleEndian::WriteUint16(aFcf, bytes);

    length = CalculateAddrFieldSize(aFcf);
    OT_ASSERT(length != kInvalidSize);

    // Security Header
    if (aFcf & kFcfSecurityEnabled)
    {
        bytes[length] = aSecurityControl;

        length += CalculateSecurityHeaderSize(aSecurityControl);
        length += CalculateMicSize(aSecurityControl);
    }

    // Command ID
    if ((aFcf & kFcfFrameTypeMask) == kFcfFrameMacCmd)
    {
        length += kCommandIdSize;
    }

    // FCS
    length += GetFcsSize();

    SetPsduLength(length);
}

uint16_t Frame::GetFrameControlField(void) const
{
    return Encoding::LittleEndian::ReadUint16(GetPsdu());
}

otError Frame::ValidatePsdu(void) const
{
    otError error  = OT_ERROR_NONE;
    uint8_t offset = FindPayloadIndex();

    VerifyOrExit(offset != kInvalidIndex, error = OT_ERROR_PARSE);
    VerifyOrExit((offset + GetFooterLength()) <= GetPsduLength(), error = OT_ERROR_PARSE);

exit:
    return error;
}

void Frame::SetAckRequest(bool aAckRequest)
{
    if (aAckRequest)
    {
        GetPsdu()[0] |= kFcfAckRequest;
    }
    else
    {
        GetPsdu()[0] &= ~kFcfAckRequest;
    }
}

void Frame::SetFramePending(bool aFramePending)
{
    if (aFramePending)
    {
        GetPsdu()[0] |= kFcfFramePending;
    }
    else
    {
        GetPsdu()[0] &= ~kFcfFramePending;
    }
}

uint8_t Frame::FindDstPanIdIndex(void) const
{
    uint8_t index = 0;

    VerifyOrExit((GetFrameControlField() & kFcfDstAddrMask) != kFcfDstAddrNone, index = kInvalidIndex);

    //  Frame Control Field and Sequence Number
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
    aPanId = Encoding::LittleEndian::ReadUint16(GetPsdu() + index);

exit:
    return error;
}

void Frame::SetDstPanId(PanId aPanId)
{
    uint8_t index = FindDstPanIdIndex();

    OT_ASSERT(index != kInvalidIndex);
    Encoding::LittleEndian::WriteUint16(aPanId, GetPsdu() + index);
}

uint8_t Frame::FindDstAddrIndex(void) const
{
#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    return kFcfSize + kDsnSize + (IsDstPanIdPresent() ? sizeof(PanId) : 0);
#else
    return kFcfSize + kDsnSize + sizeof(PanId);
#endif
}

otError Frame::GetDstAddr(Address &aAddress) const
{
    otError error = OT_ERROR_NONE;
    uint8_t index = FindDstAddrIndex();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);

    switch (GetFrameControlField() & kFcfDstAddrMask)
    {
    case kFcfDstAddrShort:
        aAddress.SetShort(Encoding::LittleEndian::ReadUint16(GetPsdu() + index));
        break;

    case kFcfDstAddrExt:
        aAddress.SetExtended(GetPsdu() + index, ExtAddress::kReverseByteOrder);
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
    Encoding::LittleEndian::WriteUint16(aShortAddress, GetPsdu() + FindDstAddrIndex());
}

void Frame::SetDstAddr(const ExtAddress &aExtAddress)
{
    uint8_t index = FindDstAddrIndex();

    OT_ASSERT((GetFrameControlField() & kFcfDstAddrMask) == kFcfDstAddrExt);
    OT_ASSERT(index != kInvalidIndex);

    aExtAddress.CopyTo(GetPsdu() + index, ExtAddress::kReverseByteOrder);
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

    VerifyOrExit((fcf & kFcfDstAddrMask) != kFcfDstAddrNone || (fcf & kFcfSrcAddrMask) != kFcfSrcAddrNone,
                 index = kInvalidIndex);

    // Frame Control Field and Sequence Number
    index += kFcfSize + kDsnSize;

    if ((fcf & kFcfPanidCompression) == 0)
    {
        // Destination PAN + Address
        switch (fcf & kFcfDstAddrMask)
        {
        case kFcfDstAddrShort:
            index += sizeof(PanId) + sizeof(ShortAddress);
            break;

        case kFcfDstAddrExt:
            index += sizeof(PanId) + sizeof(ExtAddress);
            break;
        }
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
    aPanId = Encoding::LittleEndian::ReadUint16(GetPsdu() + index);

exit:
    return error;
}

otError Frame::SetSrcPanId(PanId aPanId)
{
    otError error = OT_ERROR_NONE;
    uint8_t index = FindSrcPanIdIndex();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);
    Encoding::LittleEndian::WriteUint16(aPanId, GetPsdu() + index);

exit:
    return error;
}

uint8_t Frame::FindSrcAddrIndex(void) const
{
    uint8_t  index = 0;
    uint16_t fcf   = GetFrameControlField();

    // Frame Control Field and Sequence Number
    index += kFcfSize + kDsnSize;

    // Destination PAN
    if (IsDstPanIdPresent(fcf))
    {
        index += sizeof(PanId);
    }

    // Destination Address
    switch (fcf & kFcfDstAddrMask)
    {
    case kFcfDstAddrShort:
        index += sizeof(ShortAddress);
        break;

    case kFcfDstAddrExt:
        index += sizeof(ExtAddress);
        break;
    }

    // Source PAN
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
        aAddress.SetShort(Encoding::LittleEndian::ReadUint16(GetPsdu() + index));
        break;

    case kFcfSrcAddrExt:
        aAddress.SetExtended(GetPsdu() + index, ExtAddress::kReverseByteOrder);
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

    Encoding::LittleEndian::WriteUint16(aShortAddress, GetPsdu() + index);
}

void Frame::SetSrcAddr(const ExtAddress &aExtAddress)
{
    uint8_t index = FindSrcAddrIndex();

    OT_ASSERT((GetFrameControlField() & kFcfSrcAddrMask) == kFcfSrcAddrExt);
    OT_ASSERT(index != kInvalidIndex);

    aExtAddress.CopyTo(GetPsdu() + index, ExtAddress::kReverseByteOrder);
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
        OT_UNREACHABLE_CODE(break);
    }
}

otError Frame::GetSecurityControlField(uint8_t &aSecurityControlField) const
{
    otError error = OT_ERROR_NONE;
    uint8_t index = FindSecurityHeaderIndex();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);

    aSecurityControlField = GetPsdu()[index];

exit:
    return error;
}

void Frame::SetSecurityControlField(uint8_t aSecurityControlField)
{
    uint8_t index = FindSecurityHeaderIndex();

    OT_ASSERT(index != kInvalidIndex);

    GetPsdu()[index] = aSecurityControlField;
}

uint8_t Frame::FindSecurityHeaderIndex(void) const
{
    uint8_t index;

    VerifyOrExit(kFcfSize < GetLength(), index = kInvalidIndex);
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

    aSecurityLevel = GetPsdu()[index] & kSecLevelMask;

exit:
    return error;
}

otError Frame::GetKeyIdMode(uint8_t &aKeyIdMode) const
{
    otError error = OT_ERROR_NONE;
    uint8_t index = FindSecurityHeaderIndex();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);

    aKeyIdMode = GetPsdu()[index] & kKeyIdModeMask;

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

    aFrameCounter = Encoding::LittleEndian::ReadUint32(GetPsdu() + index);

exit:
    return error;
}

void Frame::SetFrameCounter(uint32_t aFrameCounter)
{
    uint8_t index = FindSecurityHeaderIndex();

    OT_ASSERT(index != kInvalidIndex);

    // Security Control
    index += kSecurityControlSize;

    Encoding::LittleEndian::WriteUint32(aFrameCounter, GetPsdu() + index);
}

const uint8_t *Frame::GetKeySource(void) const
{
    uint8_t        index = FindSecurityHeaderIndex();
    const uint8_t *buf   = GetPsdu() + index;

    OT_ASSERT(index != kInvalidIndex);

    // Security Control
    buf += kSecurityControlSize + kFrameCounterSize;

    return buf;
}

uint8_t Frame::GetKeySourceLength(uint8_t aKeyIdMode)
{
    uint8_t rval = 0;

    switch (aKeyIdMode)
    {
    case kKeyIdMode0:
        rval = kKeySourceSizeMode0;
        break;

    case kKeyIdMode1:
        rval = kKeySourceSizeMode1;
        break;

    case kKeyIdMode2:
        rval = kKeySourceSizeMode2;
        break;

    case kKeyIdMode3:
        rval = kKeySourceSizeMode3;
        break;
    }

    return rval;
}

void Frame::SetKeySource(const uint8_t *aKeySource)
{
    uint8_t  keySourceLength;
    uint8_t  index = FindSecurityHeaderIndex();
    uint8_t *buf   = GetPsdu() + index;

    OT_ASSERT(index != kInvalidIndex);

    keySourceLength = GetKeySourceLength(buf[0] & kKeyIdModeMask);

    buf += kSecurityControlSize + kFrameCounterSize;

    memcpy(buf, aKeySource, keySourceLength);
}

otError Frame::GetKeyId(uint8_t &aKeyId) const
{
    otError        error = OT_ERROR_NONE;
    uint8_t        keySourceLength;
    uint8_t        index = FindSecurityHeaderIndex();
    const uint8_t *buf   = GetPsdu() + index;

    VerifyOrExit(index != kInvalidIndex, OT_NOOP);

    keySourceLength = GetKeySourceLength(buf[0] & kKeyIdModeMask);

    buf += kSecurityControlSize + kFrameCounterSize + keySourceLength;

    aKeyId = buf[0];

exit:
    return error;
}

void Frame::SetKeyId(uint8_t aKeyId)
{
    uint8_t  keySourceLength;
    uint8_t  index = FindSecurityHeaderIndex();
    uint8_t *buf   = GetPsdu() + index;

    OT_ASSERT(index != kInvalidIndex);

    keySourceLength = GetKeySourceLength(buf[0] & kKeyIdModeMask);

    buf += kSecurityControlSize + kFrameCounterSize + keySourceLength;

    buf[0] = aKeyId;
}

otError Frame::GetCommandId(uint8_t &aCommandId) const
{
    otError error = OT_ERROR_NONE;
    uint8_t index = FindPayloadIndex();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);

    aCommandId = (GetPsdu() + index)[-1];

exit:
    return error;
}

otError Frame::SetCommandId(uint8_t aCommandId)
{
    otError error = OT_ERROR_NONE;
    uint8_t index = FindPayloadIndex();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);

    (GetPsdu() + index)[-1] = aCommandId;

exit:
    return error;
}

bool Frame::IsDataRequestCommand(void) const
{
    bool    isDataRequest = false;
    uint8_t commandId     = 0;

    VerifyOrExit(GetType() == kFcfFrameMacCmd, OT_NOOP);
    SuccessOrExit(GetCommandId(commandId));
    isDataRequest = (commandId == kMacCmdDataRequest);

exit:
    return isDataRequest;
}

uint8_t Frame::GetHeaderLength(void) const
{
    return static_cast<uint8_t>(GetPayload() - GetPsdu());
}

uint8_t Frame::GetFooterLength(void) const
{
    uint8_t footerLength = static_cast<uint8_t>(GetFcsSize());
    uint8_t index        = FindSecurityHeaderIndex();

    VerifyOrExit(index != kInvalidIndex, OT_NOOP);
    footerLength += CalculateMicSize((GetPsdu() + index)[0]);

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
    return GetPsduLength() - (GetHeaderLength() + GetFooterLength());
}

void Frame::SetPayloadLength(uint16_t aLength)
{
    SetPsduLength(GetHeaderLength() + GetFooterLength() + aLength);
}

uint8_t Frame::SkipSecurityHeaderIndex(void) const
{
    uint8_t index = SkipAddrFieldIndex();

    VerifyOrExit(index != kInvalidIndex, OT_NOOP);

    if (GetSecurityEnabled())
    {
        uint8_t securityControl;
        uint8_t headerSize;

        VerifyOrExit(index < GetPsduLength(), index = kInvalidIndex);
        securityControl = *(GetPsdu() + index);

        headerSize = CalculateSecurityHeaderSize(securityControl);
        VerifyOrExit(headerSize != kInvalidSize, index = kInvalidIndex);

        index += headerSize;

        VerifyOrExit(index <= GetPsduLength(), index = kInvalidIndex);
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

    VerifyOrExit(kFcfSize + kDsnSize + GetFcsSize() <= GetPsduLength(), index = kInvalidIndex);

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
        OT_UNREACHABLE_CODE(break);
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
        OT_UNREACHABLE_CODE(break);
    }

exit:
    return size;
}

uint8_t Frame::FindPayloadIndex(void) const
{
    uint8_t index = SkipSecurityHeaderIndex();
#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    const uint8_t *cur    = nullptr;
    const uint8_t *footer = GetFooter();
#endif

    VerifyOrExit(index != kInvalidIndex, OT_NOOP);

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    cur = GetPsdu() + index;

    if (IsIePresent())
    {
        while (cur + sizeof(HeaderIe) <= footer)
        {
            const HeaderIe *ie  = reinterpret_cast<const HeaderIe *>(cur);
            uint8_t         len = static_cast<uint8_t>(ie->GetLength());

            cur += sizeof(HeaderIe);
            index += sizeof(HeaderIe);

            VerifyOrExit(cur + len <= footer, index = kInvalidIndex);

            cur += len;
            index += len;

            if (ie->GetId() == kHeaderIeTermination2)
            {
                break;
            }
        }

        // Assume no Payload IE in current implementation
    }
#endif

    // Command ID
    if ((GetFrameControlField() & kFcfFrameTypeMask) == kFcfFrameMacCmd)
    {
        index += kCommandIdSize;
    }

exit:
    return index;
}

const uint8_t *Frame::GetPayload(void) const
{
    uint8_t        index   = FindPayloadIndex();
    const uint8_t *payload = GetPsdu() + index;

    VerifyOrExit(index != kInvalidIndex, payload = nullptr);

exit:
    return payload;
}

const uint8_t *Frame::GetFooter(void) const
{
    return GetPsdu() + GetPsduLength() - GetFooterLength();
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
    uint8_t  index = FindHeaderIeIndex();
    uint8_t *cur;
    uint8_t *base;

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_NOT_FOUND);
    cur  = GetPsdu() + index;
    base = cur;

    for (uint8_t i = 0; i < aIeCount; i++)
    {
        memcpy(cur, &aIeList[i], sizeof(HeaderIe));
        cur += sizeof(HeaderIe);
        cur += aIeList[i].GetLength();
    }

    SetPsduLength(GetPsduLength() + static_cast<uint16_t>(cur - base));

exit:
    return error;
}

const uint8_t *Frame::GetHeaderIe(uint8_t aIeId) const
{
    uint8_t        index   = FindHeaderIeIndex();
    const uint8_t *cur     = nullptr;
    const uint8_t *payload = GetPayload();

    VerifyOrExit(index != kInvalidIndex, OT_NOOP);

    cur = GetPsdu() + index;

    while (cur + sizeof(HeaderIe) <= payload)
    {
        const HeaderIe *ie  = reinterpret_cast<const HeaderIe *>(cur);
        uint8_t         len = static_cast<uint8_t>(ie->GetLength());

        if (ie->GetId() == aIeId)
        {
            break;
        }

        cur += sizeof(HeaderIe);

        VerifyOrExit(cur + len <= payload, cur = nullptr);

        cur += len;
    }

    if (cur == payload)
    {
        cur = nullptr;
    }

exit:
    return cur;
}
#endif // OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
void Frame::SetCslIe(uint16_t aCslPeriod, uint16_t aCslPhase)
{
    uint8_t *cur = GetHeaderIe(Frame::kHeaderIeCsl);
    CslIe *  csl;

    OT_ASSERT(cur != nullptr);

    csl = reinterpret_cast<CslIe *>(cur + sizeof(HeaderIe));
    csl->SetPeriod(aCslPeriod);
    csl->SetPhase(aCslPhase);
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
const TimeIe *Frame::GetTimeIe(void) const
{
    const TimeIe * timeIe                              = nullptr;
    const uint8_t *cur                                 = nullptr;
    uint8_t        oui[VendorIeHeader::kVendorOuiSize] = {VendorIeHeader::kVendorOuiNest & 0xff,
                                                   (VendorIeHeader::kVendorOuiNest >> 8) & 0xff,
                                                   (VendorIeHeader::kVendorOuiNest >> 16) & 0xff};

    cur = GetHeaderIe(kHeaderIeVendor);
    VerifyOrExit(cur != nullptr, OT_NOOP);

    cur += sizeof(HeaderIe);

    timeIe = reinterpret_cast<const TimeIe *>(cur);
    VerifyOrExit(memcmp(oui, timeIe->GetVendorOui(), VendorIeHeader::kVendorOuiSize) == 0, timeIe = nullptr);
    VerifyOrExit(timeIe->GetSubType() == VendorIeHeader::kVendorIeTime, timeIe = nullptr);

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

    memcpy(mPsdu, aFromFrame.mPsdu, aFromFrame.GetPsduLength());

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
    Encoding::LittleEndian::WriteUint16(fcf, mPsdu);

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

    Encoding::LittleEndian::WriteUint16(fcf, mPsdu);

    // Set sequence number
    mPsdu[kSequenceIndex] = aFrame.GetSequence();

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

    if (IsDstPanIdPresent())
    {
        SetDstPanId(panId);
    }

    if (aFrame.IsSrcAddrPresent())
    {
        SuccessOrExit(error = aFrame.GetSrcAddr(address));
        SetDstAddr(address);
    }

    SetPsduLength(kMaxPsduSize); // At this time the length of ACK hasn't been determined, set it to
                                 // `kMaxPsduSize` to call methods that check frame length.

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
        memcpy(GetPsdu() + FindHeaderIeIndex(), aIeData, aIeLength);
    }

    // Set frame length
    footerLength = GetFooterLength();
    OT_ASSERT(footerLength != kInvalidIndex);
    mLength = SkipSecurityHeaderIndex() + aIeLength + GetFooterLength();

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2

// LCOV_EXCL_START

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_NOTE) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

Frame::InfoString Frame::ToInfoString(void) const
{
    InfoString string;
    uint8_t    commandId, type;
    Address    src, dst;

    IgnoreError(string.Append("len:%d, seqnum:%d, type:", GetLength(), GetSequence()));

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
