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
#include "utils/wrap_string.h"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/random.hpp"

namespace ot {
namespace Mac {

void ExtAddress::GenerateRandom(void)
{
    Random::FillBuffer(m8, sizeof(ExtAddress));
    SetGroup(false);
    SetLocal(true);
}

bool ExtAddress::operator==(const ExtAddress &aOther) const
{
    return memcmp(m8, aOther.m8, sizeof(ExtAddress)) == 0;
}

bool ExtAddress::operator!=(const ExtAddress &aOther) const
{
    return memcmp(m8, aOther.m8, sizeof(ExtAddress)) != 0;
}

ExtAddress::InfoString ExtAddress::ToString(void) const
{
    return InfoString("%02x%02x%02x%02x%02x%02x%02x%02x", m8[0], m8[1], m8[2], m8[3], m8[4], m8[5], m8[6], m8[7]);
}

void Address::SetExtended(const uint8_t *aBuffer, bool aReverse)
{
    mType = kTypeExtended;

    if (aReverse)
    {
        for (unsigned int i = 0; i < sizeof(ExtAddress); i++)
        {
            mShared.mExtAddress.m8[i] = aBuffer[sizeof(ExtAddress) - 1 - i];
        }
    }
    else
    {
        memcpy(mShared.mExtAddress.m8, aBuffer, sizeof(ExtAddress));
    }
}

Address::InfoString Address::ToString(void) const
{
    return (mType == kTypeExtended) ? GetExtended().ToString()
                                    : (mType == kTypeNone ? InfoString("None") : InfoString("0x%04x", GetShort()));
}

otError Frame::InitMacHeader(uint16_t aFcf, uint8_t aSecurityControl)
{
    uint8_t *bytes  = GetPsdu();
    uint8_t  length = 0;

    // Frame Control Field
    Encoding::LittleEndian::WriteUint16(aFcf, bytes);
    length += kFcfSize;

    // Sequence Number
    length += kDsnSize;

    // Destination PAN + Address
    switch (aFcf & kFcfDstAddrMask)
    {
    case kFcfDstAddrNone:
        break;

    case kFcfDstAddrShort:
        length += sizeof(PanId) + sizeof(ShortAddress);
        break;

    case kFcfDstAddrExt:
        length += sizeof(PanId) + sizeof(ExtAddress);
        break;

    default:
        assert(false);
    }

    // Source PAN
    if (IsSrcPanIdPresent(aFcf))
    {
        length += sizeof(PanId);
    }

    // Source Address
    switch (aFcf & kFcfSrcAddrMask)
    {
    case kFcfSrcAddrNone:
        break;

    case kFcfSrcAddrShort:
        length += sizeof(ShortAddress);
        break;

    case kFcfSrcAddrExt:
        length += sizeof(ExtAddress);
        break;

    default:
        assert(false);
    }

    // Security Header
    if (aFcf & kFcfSecurityEnabled)
    {
        bytes[length] = aSecurityControl;

        if (aSecurityControl & kSecLevelMask)
        {
            length += kSecurityControlSize + kFrameCounterSize;
        }

        switch (aSecurityControl & kKeyIdModeMask)
        {
        case kKeyIdMode0:
            length += kKeySourceSizeMode0;
            break;

        case kKeyIdMode1:
            length += kKeySourceSizeMode1 + kKeyIndexSize;
            break;

        case kKeyIdMode2:
            length += kKeySourceSizeMode2 + kKeyIndexSize;
            break;

        case kKeyIdMode3:
            length += kKeySourceSizeMode3 + kKeyIndexSize;
            break;
        }
    }

    // Command ID
    if ((aFcf & kFcfFrameTypeMask) == kFcfFrameMacCmd)
    {
        length += kCommandIdSize;
    }

    SetPsduLength(length + GetFooterLength());

    return OT_ERROR_NONE;
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

otError Frame::GetDstPanId(PanId &aPanId) const
{
    otError error = OT_ERROR_NONE;
    uint8_t index = FindDstPanIdIndex();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);
    aPanId = Encoding::LittleEndian::ReadUint16(GetPsdu() + index);

exit:
    return error;
}

otError Frame::SetDstPanId(PanId aPanId)
{
    uint8_t index = FindDstPanIdIndex();

    assert(index != kInvalidIndex);
    Encoding::LittleEndian::WriteUint16(aPanId, GetPsdu() + index);

    return OT_ERROR_NONE;
}

uint8_t Frame::FindDstAddrIndex(void) const
{
    return kFcfSize + kDsnSize + sizeof(PanId);
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
        aAddress.SetExtended(GetPsdu() + index, /* reverse */ true);
        break;

    default:
        aAddress.SetNone();
        break;
    }

exit:
    return error;
}

otError Frame::SetDstAddr(ShortAddress aShortAddress)
{
    assert((GetFrameControlField() & kFcfDstAddrMask) == kFcfDstAddrShort);
    Encoding::LittleEndian::WriteUint16(aShortAddress, GetPsdu() + FindDstAddrIndex());

    return OT_ERROR_NONE;
}

otError Frame::SetDstAddr(const ExtAddress &aExtAddress)
{
    uint8_t  index = FindDstAddrIndex();
    uint8_t *buf   = GetPsdu() + index;

    assert((GetFrameControlField() & kFcfDstAddrMask) == kFcfDstAddrExt);
    assert(index != kInvalidIndex);

    for (unsigned int i = 0; i < sizeof(ExtAddress); i++)
    {
        buf[i] = aExtAddress.m8[sizeof(ExtAddress) - 1 - i];
    }

    return OT_ERROR_NONE;
}

otError Frame::SetDstAddr(const Address &aAddress)
{
    otError error = OT_ERROR_NONE;

    switch (aAddress.GetType())
    {
    case Address::kTypeShort:
        error = SetDstAddr(aAddress.GetShort());
        break;

    case Address::kTypeExtended:
        error = SetDstAddr(aAddress.GetExtended());
        break;

    default:
        assert(false);
        break;
    }

    return error;
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

bool Frame::IsSrcPanIdPresent(uint16_t aFcf) const
{
    bool srcPanIdPresent = false;

    if ((aFcf & kFcfSrcAddrMask) != kFcfSrcAddrNone && (aFcf & kFcfPanidCompression) == 0)
    {
#if OPENTHREAD_CONFIG_HEADER_IE_SUPPORT
        // Handle a special case in IEEE 802.15.4-2015, when Pan ID Compression is 0, but Src Pan ID is not present:
        //  Dest Address:       Extended
        //  Source Address:     Extended
        //  Dest Pan ID:        Present
        //  Src Pan ID:         Not Present
        //  Pan ID Compression: 0
        if ((aFcf & kFcfFrameVersionMask) != kFcfFrameVersion2015 || (aFcf & kFcfDstAddrMask) != kFcfDstAddrExt ||
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

    // Source PAN
    if (IsSrcPanIdPresent(fcf))
    {
        index += sizeof(PanId);
    }

    return index;
}

otError Frame::GetSrcAddr(Address &address) const
{
    otError  error = OT_ERROR_NONE;
    uint8_t  index = FindSrcAddrIndex();
    uint16_t fcf   = GetFrameControlField();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);

    switch (fcf & kFcfSrcAddrMask)
    {
    case kFcfSrcAddrShort:
        address.SetShort(Encoding::LittleEndian::ReadUint16(GetPsdu() + index));
        break;

    case kFcfSrcAddrExt:
        address.SetExtended(GetPsdu() + index, /* reverse */ true);
        break;

    default:
        address.SetNone();
        break;
    }

exit:
    return error;
}

otError Frame::SetSrcAddr(ShortAddress aShortAddress)
{
    uint8_t index = FindSrcAddrIndex();

    assert((GetFrameControlField() & kFcfSrcAddrMask) == kFcfSrcAddrShort);
    assert(index != kInvalidIndex);

    Encoding::LittleEndian::WriteUint16(aShortAddress, GetPsdu() + index);

    return OT_ERROR_NONE;
}

otError Frame::SetSrcAddr(const ExtAddress &aExtAddress)
{
    uint8_t  index = FindSrcAddrIndex();
    uint8_t *buf   = GetPsdu() + index;

    assert((GetFrameControlField() & kFcfSrcAddrMask) == kFcfSrcAddrExt);
    assert(index != kInvalidIndex);

    for (unsigned int i = 0; i < sizeof(aExtAddress); i++)
    {
        buf[i] = aExtAddress.m8[sizeof(aExtAddress) - 1 - i];
    }

    return OT_ERROR_NONE;
}

otError Frame::SetSrcAddr(const Address &aAddress)
{
    otError error = OT_ERROR_NONE;

    switch (aAddress.GetType())
    {
    case Address::kTypeShort:
        error = SetSrcAddr(aAddress.GetShort());
        break;

    case Address::kTypeExtended:
        error = SetSrcAddr(aAddress.GetExtended());
        break;

    default:
        assert(false);
        break;
    }

    return error;
}

uint8_t Frame::FindSecurityHeaderIndex(void) const
{
    uint8_t  index = 0;
    uint16_t fcf   = GetFrameControlField();

    VerifyOrExit((fcf & kFcfSecurityEnabled) != 0, index = kInvalidIndex);

    // Frame Control Field and  Sequence Number
    index += kFcfSize + kDsnSize;

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

    // Source PAN
    if (IsSrcPanIdPresent(fcf))
    {
        index += sizeof(PanId);
    }

    // Source Address
    switch (fcf & kFcfSrcAddrMask)
    {
    case kFcfSrcAddrShort:
        index += sizeof(ShortAddress);
        break;

    case kFcfSrcAddrExt:
        index += sizeof(ExtAddress);
        break;
    }

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

otError Frame::SetFrameCounter(uint32_t aFrameCounter)
{
    uint8_t index = FindSecurityHeaderIndex();

    assert(index != kInvalidIndex);

    // Security Control
    index += kSecurityControlSize;

    Encoding::LittleEndian::WriteUint32(aFrameCounter, GetPsdu() + index);

    return OT_ERROR_NONE;
}

const uint8_t *Frame::GetKeySource(void) const
{
    uint8_t        index = FindSecurityHeaderIndex();
    const uint8_t *buf   = GetPsdu() + index;

    assert(index != kInvalidIndex);

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

    assert(index != kInvalidIndex);

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

    VerifyOrExit(index != kInvalidIndex);

    keySourceLength = GetKeySourceLength(buf[0] & kKeyIdModeMask);

    buf += kSecurityControlSize + kFrameCounterSize + keySourceLength;

    aKeyId = buf[0];

exit:
    return error;
}

otError Frame::SetKeyId(uint8_t aKeyId)
{
    uint8_t  keySourceLength;
    uint8_t  index = FindSecurityHeaderIndex();
    uint8_t *buf   = GetPsdu() + index;

    assert(index != kInvalidIndex);

    keySourceLength = GetKeySourceLength(buf[0] & kKeyIdModeMask);

    buf += kSecurityControlSize + kFrameCounterSize + keySourceLength;

    buf[0] = aKeyId;

    return OT_ERROR_NONE;
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

    VerifyOrExit(GetType() == kFcfFrameMacCmd);
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
    uint8_t footerLength = 0;
    uint8_t index        = FindSecurityHeaderIndex();

    VerifyOrExit(index != kInvalidIndex);

    switch ((GetPsdu() + index)[0] & kSecLevelMask)
    {
    case kSecNone:
    case kSecEnc:
        footerLength += kMic0Size;
        break;

    case kSecMic32:
    case kSecEncMic32:
        footerLength += kMic32Size;
        break;

    case kSecMic64:
    case kSecEncMic64:
        footerLength += kMic64Size;
        break;

    case kSecMic128:
    case kSecEncMic128:
        footerLength += kMic128Size;
        break;
    }

exit:
    // Frame Check Sequence
    footerLength += kFcsSize;

    return footerLength;
}

uint8_t Frame::GetMaxPayloadLength(void) const
{
    return kMTU - (GetHeaderLength() + GetFooterLength());
}

uint8_t Frame::GetPayloadLength(void) const
{
    return GetPsduLength() - (GetHeaderLength() + GetFooterLength());
}

otError Frame::SetPayloadLength(uint8_t aLength)
{
    SetPsduLength(GetHeaderLength() + GetFooterLength() + aLength);
    return OT_ERROR_NONE;
}

uint8_t Frame::SkipSecurityHeaderIndex(void) const
{
    uint8_t  index = 0;
    uint16_t fcf;

    // Frame Control
    index += kFcfSize;
    // Sequence Number
    index += kDsnSize;

    VerifyOrExit((index + kFcsSize) <= GetPsduLength(), index = kInvalidIndex);

    fcf = GetFrameControlField();

    // Destination PAN + Address
    switch (fcf & kFcfDstAddrMask)
    {
    case kFcfDstAddrNone:
        break;

    case kFcfDstAddrShort:
        index += sizeof(PanId) + sizeof(ShortAddress);
        break;

    case kFcfDstAddrExt:
        index += sizeof(PanId) + sizeof(ExtAddress);
        break;

    default:
        ExitNow(index = kInvalidIndex);
    }

    // Source PAN
    if (IsSrcPanIdPresent(fcf))
    {
        index += sizeof(PanId);
    }

    // Source Address
    switch (fcf & kFcfSrcAddrMask)
    {
    case kFcfSrcAddrNone:
        break;

    case kFcfSrcAddrShort:
        index += sizeof(ShortAddress);
        break;

    case kFcfSrcAddrExt:
        index += sizeof(ExtAddress);
        break;

    default:
        ExitNow(index = kInvalidIndex);
    }

    VerifyOrExit((index + kFcsSize) <= GetPsduLength(), index = kInvalidIndex);

    // Security Control + Frame Counter + Key Identifier
    if ((fcf & kFcfSecurityEnabled) != 0)
    {
        uint8_t securityControl = *(GetPsdu() + index);

        index += kSecurityControlSize + kFrameCounterSize;

        switch (securityControl & kKeyIdModeMask)
        {
        case kKeyIdMode0:
            index += kKeySourceSizeMode0;
            break;

        case kKeyIdMode1:
            index += kKeySourceSizeMode1 + kKeyIndexSize;
            break;

        case kKeyIdMode2:
            index += kKeySourceSizeMode2 + kKeyIndexSize;
            break;

        case kKeyIdMode3:
            index += kKeySourceSizeMode3 + kKeyIndexSize;
            break;
        }
    }

exit:
    return index;
}

uint8_t Frame::FindPayloadIndex(void) const
{
    uint8_t index = SkipSecurityHeaderIndex();
#if OPENTHREAD_CONFIG_HEADER_IE_SUPPORT
    const uint8_t *cur    = NULL;
    const uint8_t *footer = GetFooter();
#endif

    VerifyOrExit(index != kInvalidIndex);

#if OPENTHREAD_CONFIG_HEADER_IE_SUPPORT
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

    VerifyOrExit(index != kInvalidIndex, payload = NULL);

exit:
    return payload;
}

const uint8_t *Frame::GetFooter(void) const
{
    return GetPsdu() + GetPsduLength() - GetFooterLength();
}

#if OPENTHREAD_CONFIG_HEADER_IE_SUPPORT
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

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_FAILED);
    cur  = GetPsdu() + index;
    base = cur;

    for (uint8_t i = 0; i < aIeCount; i++)
    {
        memcpy(cur, &aIeList[i], sizeof(HeaderIe));
        cur += sizeof(HeaderIe);
        cur += aIeList[i].GetLength();
    }

    SetPsduLength(GetPsduLength() + static_cast<uint8_t>(cur - base));

exit:
    return error;
}

const uint8_t *Frame::GetHeaderIe(uint8_t aIeId) const
{
    uint8_t        index   = FindHeaderIeIndex();
    const uint8_t *cur     = NULL;
    const uint8_t *payload = GetPayload();

    VerifyOrExit(index != kInvalidIndex);

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

        VerifyOrExit(cur + len <= payload, cur = NULL);

        cur += len;
    }

    if (cur == payload)
    {
        cur = NULL;
    }

exit:
    return cur;
}
#endif // OPENTHREAD_CONFIG_HEADER_IE_SUPPORT

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
const uint8_t *Frame::GetTimeIe(void) const
{
    const TimeIe * timeIe       = NULL;
    const uint8_t *cur          = NULL;
    uint8_t oui[kVendorOuiSize] = {kVendorOuiNest & 0xff, (kVendorOuiNest >> 8) & 0xff, (kVendorOuiNest >> 16) & 0xff};

    cur = GetHeaderIe(kHeaderIeVendor);
    VerifyOrExit(cur != NULL);

    cur += sizeof(HeaderIe);

    timeIe = reinterpret_cast<const TimeIe *>(cur);
    VerifyOrExit(memcmp(oui, timeIe->GetVendorOui(), kVendorOuiSize) == 0, cur = NULL);
    VerifyOrExit(timeIe->GetSubType() == kVendorIeTime, cur = NULL);

exit:
    return cur;
}
#endif // OPENTHREAD_CONFIG_ENABLE_TIME_SYNC

void Frame::CopyFrom(const Frame &aAnotherFrame)
{
    uint8_t *      psduBuffer   = mPsdu;
    otRadioIeInfo *ieInfoBuffer = mIeInfo;

    memcpy(this, &aAnotherFrame, sizeof(Frame));

    // Set the original buffer pointers back on the frame
    // which were overwritten by above `memcpy()`.

    mPsdu   = psduBuffer;
    mIeInfo = ieInfoBuffer;

    memcpy(mPsdu, aAnotherFrame.mPsdu, aAnotherFrame.GetPsduLength());
    memcpy(mIeInfo, aAnotherFrame.mIeInfo, sizeof(otRadioIeInfo));
}

Frame::InfoString Frame::ToInfoString(void) const
{
    InfoString string;
    uint8_t    commandId, type;
    Address    src, dst;

    string.Append("len:%d, seqnum:%d, type:", GetLength(), GetSequence());

    type = GetType();

    switch (type)
    {
    case kFcfFrameBeacon:
        string.Append("Beacon");
        break;

    case kFcfFrameData:
        string.Append("Data");
        break;

    case kFcfFrameAck:
        string.Append("Ack");
        break;

    case kFcfFrameMacCmd:
        if (GetCommandId(commandId) != OT_ERROR_NONE)
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

    GetSrcAddr(src);
    GetDstAddr(dst);

    string.Append(", src:%s, dst:%s, sec:%s, ackreq:%s", src.ToString().AsCString(), dst.ToString().AsCString(),
                  GetSecurityEnabled() ? "yes" : "no", GetAckRequest() ? "yes" : "no");

    return string;
}

BeaconPayload::InfoString BeaconPayload::ToInfoString(void) const
{
    const uint8_t *xpanid = GetExtendedPanId();

    return InfoString("name:%s, xpanid:%02x%02x%02x%02x%02x%02x%02x%02x, id:%d ver:%d, joinable:%s, native:%s",
                      GetNetworkName(), xpanid[0], xpanid[1], xpanid[2], xpanid[3], xpanid[4], xpanid[5], xpanid[6],
                      xpanid[7], GetProtocolId(), GetProtocolVersion(), IsJoiningPermitted() ? "yes" : "no",
                      IsNative() ? "yes" : "no");
}

} // namespace Mac
} // namespace ot
