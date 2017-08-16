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

#include <openthread/config.h>

#include "mac_frame.hpp"

#include <stdio.h>
#include "utils/wrap_string.h"

#include "common/code_utils.hpp"
#include "common/debug.hpp"

namespace ot {
namespace Mac {

const char *Address::ToString(char *aBuf, uint16_t aSize) const
{
    switch (mLength)
    {
    case sizeof(ShortAddress):
        snprintf(aBuf, aSize, "0x%04x", mShortAddress);
        break;

    case sizeof(ExtAddress):
        snprintf(aBuf, aSize, "%02x%02x%02x%02x%02x%02x%02x%02x",
                 mExtAddress.m8[0], mExtAddress.m8[1], mExtAddress.m8[2], mExtAddress.m8[3],
                 mExtAddress.m8[4], mExtAddress.m8[5], mExtAddress.m8[6], mExtAddress.m8[7]);
        break;

    default:
        snprintf(aBuf, aSize, "None");
        break;
    }

    return aBuf;
}

otError Frame::InitMacHeader(uint16_t aFcf, uint8_t aSecurityControl)
{
    uint8_t *bytes = GetPsdu();
    uint8_t length = 0;

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

    // Source PAN + Address
    switch (aFcf & kFcfSrcAddrMask)
    {
    case kFcfSrcAddrNone:
        break;

    case kFcfSrcAddrShort:
        if ((aFcf & kFcfPanidCompression) == 0)
        {
            length += sizeof(PanId);
        }

        length += sizeof(ShortAddress);
        break;

    case kFcfSrcAddrExt:
        if ((aFcf & kFcfPanidCompression) == 0)
        {
            length += sizeof(PanId);
        }

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
    otError error = OT_ERROR_PARSE;
    uint8_t offset = kFcfSize + kDsnSize;
    uint8_t footerLength = kFcsSize;
    uint16_t fcf;

    VerifyOrExit((offset + footerLength) <= GetPsduLength());

    fcf = GetFrameControlField();

    // Destination PAN + Address
    switch (fcf & kFcfDstAddrMask)
    {
    case kFcfDstAddrNone:
        break;

    case kFcfDstAddrShort:
        offset += sizeof(PanId) + sizeof(ShortAddress);
        break;

    case kFcfDstAddrExt:
        offset += sizeof(PanId) + sizeof(ExtAddress);
        break;

    default:
        goto exit;
    }

    // Source PAN + Address
    switch (fcf & kFcfSrcAddrMask)
    {
    case kFcfSrcAddrNone:
        break;

    case kFcfSrcAddrShort:
        if ((fcf & kFcfPanidCompression) == 0)
        {
            offset += sizeof(PanId);
        }

        offset += sizeof(ShortAddress);
        break;

    case kFcfSrcAddrExt:
        if ((fcf & kFcfPanidCompression) == 0)
        {
            offset += sizeof(PanId);
        }

        offset += sizeof(ExtAddress);
        break;

    default:
        goto exit;
    }

    VerifyOrExit((offset + footerLength) <= GetPsduLength());

    // Security Header
    if (fcf & kFcfSecurityEnabled)
    {
        uint8_t secControl = GetPsdu()[offset];

        offset += kSecurityControlSize + kFrameCounterSize;

        switch (secControl & kKeyIdModeMask)
        {
        case kKeyIdMode0:
            offset += kKeySourceSizeMode0;
            break;

        case kKeyIdMode1:
            offset += kKeySourceSizeMode1 + kKeyIndexSize;
            break;

        case kKeyIdMode2:
            offset += kKeySourceSizeMode2 + kKeyIndexSize;
            break;

        case kKeyIdMode3:
            offset += kKeySourceSizeMode3 + kKeyIndexSize;
            break;
        }

        switch (secControl & kSecLevelMask)
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
    }

    // Command ID
    if ((fcf & kFcfFrameTypeMask) == kFcfFrameMacCmd)
    {
        offset += kCommandIdSize;
    }

    VerifyOrExit((offset + footerLength) <= GetPsduLength());

    error = OT_ERROR_NONE;

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
        aAddress.mLength = sizeof(ShortAddress);
        aAddress.mShortAddress = Encoding::LittleEndian::ReadUint16(GetPsdu() + index);
        break;

    case kFcfDstAddrExt:
    {
        const uint8_t *buf = GetPsdu() + index;

        aAddress.mLength = sizeof(ExtAddress);

        for (unsigned int i = 0; i < sizeof(ExtAddress); i++)
        {
            aAddress.mExtAddress.m8[i] = buf[sizeof(ExtAddress) - 1 - i];
        }

        break;
    }

    default:
        aAddress.mLength = 0;
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
    uint8_t index = FindDstAddrIndex();
    uint8_t *buf = GetPsdu() + index;

    assert((GetFrameControlField() & kFcfDstAddrMask) == kFcfDstAddrExt);
    assert(index != kInvalidIndex);

    for (unsigned int i = 0; i < sizeof(ExtAddress); i++)
    {
        buf[i] = aExtAddress.m8[sizeof(ExtAddress) - 1 - i];
    }

    return OT_ERROR_NONE;
}

uint8_t Frame::FindSrcPanIdIndex(void) const
{
    uint8_t index = 0;
    uint16_t fcf = GetFrameControlField();

    VerifyOrExit((fcf & kFcfDstAddrMask) != kFcfDstAddrNone ||
                 (fcf & kFcfSrcAddrMask) != kFcfSrcAddrNone, index = kInvalidIndex);

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
    uint8_t index = 0;
    uint16_t fcf = GetFrameControlField();

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
    if ((fcf & kFcfPanidCompression) == 0)
    {
        index += sizeof(PanId);
    }

    return index;
}

otError Frame::GetSrcAddr(Address &address) const
{
    otError error = OT_ERROR_NONE;
    uint8_t index = FindSrcAddrIndex();
    uint16_t fcf = GetFrameControlField();

    VerifyOrExit(index != kInvalidIndex, error = OT_ERROR_PARSE);

    switch (fcf & kFcfSrcAddrMask)
    {
    case kFcfSrcAddrShort:
        address.mLength = sizeof(ShortAddress);
        address.mShortAddress = Encoding::LittleEndian::ReadUint16(GetPsdu() + index);
        break;

    case kFcfSrcAddrExt:
    {
        const uint8_t *buf = GetPsdu() + index;

        address.mLength = sizeof(ExtAddress);

        for (unsigned int i = 0; i < sizeof(ExtAddress); i++)
        {
            address.mExtAddress.m8[i] = buf[sizeof(ExtAddress) - 1 - i];
        }

        break;
    }

    default:
        address.mLength = 0;
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
    uint8_t index = FindSrcAddrIndex();
    uint8_t *buf = GetPsdu() + index;

    assert((GetFrameControlField() & kFcfSrcAddrMask) == kFcfSrcAddrExt);
    assert(index != kInvalidIndex);

    for (unsigned int i = 0; i < sizeof(aExtAddress); i++)
    {
        buf[i] = aExtAddress.m8[sizeof(aExtAddress) - 1 - i];
    }

    return OT_ERROR_NONE;
}

uint8_t Frame::FindSecurityHeaderIndex(void) const
{
    uint8_t index = 0;
    uint16_t fcf = GetFrameControlField();

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

    // Source PAN + Address
    switch (fcf & kFcfSrcAddrMask)
    {
    case kFcfSrcAddrShort:
        if ((fcf & kFcfPanidCompression) == 0)
        {
            index += sizeof(PanId);
        }

        index += sizeof(ShortAddress);
        break;

    case kFcfSrcAddrExt:
        if ((fcf & kFcfPanidCompression) == 0)
        {
            index += sizeof(PanId);
        }

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
    uint8_t index = FindSecurityHeaderIndex();
    const uint8_t *buf = GetPsdu() + index;

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
    uint8_t keySourceLength;
    uint8_t index = FindSecurityHeaderIndex();
    uint8_t *buf = GetPsdu() + index;

    assert(index != kInvalidIndex);

    keySourceLength = GetKeySourceLength(buf[0] & kKeyIdModeMask);

    buf += kSecurityControlSize + kFrameCounterSize;

    memcpy(buf, aKeySource, keySourceLength);
}

otError Frame::GetKeyId(uint8_t &aKeyId) const
{
    otError error = OT_ERROR_NONE;
    uint8_t keySourceLength;
    uint8_t index = FindSecurityHeaderIndex();
    const uint8_t *buf = GetPsdu() + index;

    VerifyOrExit(index != kInvalidIndex);

    keySourceLength = GetKeySourceLength(buf[0] & kKeyIdModeMask);

    buf += kSecurityControlSize + kFrameCounterSize + keySourceLength;

    aKeyId = buf[0];

exit:
    return error;
}

otError Frame::SetKeyId(uint8_t aKeyId)
{
    uint8_t keySourceLength;
    uint8_t index = FindSecurityHeaderIndex();
    uint8_t *buf = GetPsdu() + index;

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
    bool isDataRequest = false;
    uint8_t commandId = 0;

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
    uint8_t index = FindSecurityHeaderIndex();

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

uint8_t Frame::FindPayloadIndex(void) const
{
    uint8_t index = 0;
    uint16_t fcf = GetFrameControlField();
    uint8_t securityControl;

    // Frame Control
    index += kFcfSize;
    // Sequence Number
    index += kDsnSize;

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

    // Source PAN + Address
    switch (fcf & kFcfSrcAddrMask)
    {
    case kFcfSrcAddrNone:
        break;

    case kFcfSrcAddrShort:
        if ((fcf & kFcfPanidCompression) == 0)
        {
            index += sizeof(PanId);
        }

        index += sizeof(ShortAddress);
        break;

    case kFcfSrcAddrExt:
        if ((fcf & kFcfPanidCompression) == 0)
        {
            index += sizeof(PanId);
        }

        index += sizeof(ExtAddress);
        break;

    default:
        ExitNow(index = kInvalidIndex);
    }

    // Security Control + Frame Counter + Key Identifier
    if ((fcf & kFcfSecurityEnabled) != 0)
    {
        securityControl = *(GetPsdu() + index);

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

    // Command ID
    if ((fcf & kFcfFrameTypeMask) == kFcfFrameMacCmd)
    {
        index += kCommandIdSize;
    }

exit:
    return index;

}

uint8_t *Frame::GetPayload(void)
{
    uint8_t index = FindPayloadIndex();
    uint8_t *payload = GetPsdu() + index;

    VerifyOrExit(index != kInvalidIndex, payload = NULL);

exit:
    return payload;
}

const uint8_t *Frame::GetPayload(void) const
{
    uint8_t index = FindPayloadIndex();
    const uint8_t *payload = GetPsdu() + index;

    VerifyOrExit(index != kInvalidIndex, payload = NULL);

exit:
    return payload;
}

uint8_t *Frame::GetFooter(void)
{
    return GetPsdu() + GetPsduLength() - GetFooterLength();
}

const uint8_t *Frame::GetFooter(void) const
{
    return GetPsdu() + GetPsduLength() - GetFooterLength();
}

const char *Frame::ToInfoString(char *aBuf, uint16_t aSize) const
{
    uint8_t type, commandId;
    Address src, dst;
    const char *typeStr;
    char stringBuffer[10];
    char srcStringBuffer[Address::kAddressStringSize];
    char dstStringBuffer[Address::kAddressStringSize];

    type = GetType();

    switch (type)
    {
    case kFcfFrameBeacon:
        typeStr = "Beacon";
        break;

    case kFcfFrameData:
        typeStr = "Data";
        break;

    case kFcfFrameAck:
        typeStr = "Ack";
        break;

    case kFcfFrameMacCmd:
        if (GetCommandId(commandId) != OT_ERROR_NONE)
        {
            commandId = 0xff;
        }

        switch (commandId)
        {
        case kMacCmdDataRequest:
            typeStr = "Cmd(DataReq)";
            break;

        case kMacCmdBeaconRequest:
            typeStr = "Cmd(BeaconReq)";
            break;

        default:
            snprintf(stringBuffer, sizeof(stringBuffer), "Cmd(%d)", commandId);
            typeStr = stringBuffer;
            break;
        }

        break;

    default:
        snprintf(stringBuffer, sizeof(stringBuffer), "%d", type);
        typeStr = stringBuffer;
        break;
    }

    if (GetSrcAddr(src) != OT_ERROR_NONE)
    {
        src.mLength = 0;
    }

    if (GetDstAddr(dst) != OT_ERROR_NONE)
    {
        dst.mLength = 0;
    }

    snprintf(aBuf, aSize, "len:%d, seqnum:%d, type:%s, src:%s, dst:%s, sec:%s, ackreq:%s", GetLength(), GetSequence(),
             typeStr, src.ToString(srcStringBuffer, sizeof(srcStringBuffer)),
             dst.ToString(dstStringBuffer, sizeof(dstStringBuffer)), GetSecurityEnabled() ? "yes" : "no",
             GetAckRequest() ? "yes" : "no");

    return aBuf;
}

const char *BeaconPayload::ToInfoString(char *aBuf, uint16_t aSize)
{
    const uint8_t *xpanid = GetExtendedPanId();

    snprintf(aBuf, aSize, "name:%s, xpanid:%02x%02x%02x%02x%02x%02x%02x%02x, id:%d ver:%d, joinable:%s, native:%s",
             GetNetworkName(), xpanid[0], xpanid[1], xpanid[2], xpanid[3], xpanid[4], xpanid[5], xpanid[6], xpanid[7],
             GetProtocolId(), GetProtocolVersion(), IsJoiningPermitted() ? "yes" : "no", IsNative() ? "yes" : "no");

    return aBuf;
}

}  // namespace Mac
}  // namespace ot

