/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <mac/mac_frame.hpp>
#include <net/ip6_address.hpp>

namespace Thread {
namespace Mac {

void ExtAddress::Set(const Ip6::Address &aIpAddress)
{
    memcpy(m8, aIpAddress.GetIid(), sizeof(m8));
    m8[0] ^= 0x02;
}

ThreadError Frame::InitMacHeader(uint16_t aFcf, uint8_t aSecurityControl)
{
    uint8_t *bytes = GetPsdu();
    uint8_t length = 0;

    // Frame Control Field
    bytes[0] = aFcf;
    bytes[1] = aFcf >> 8;
    length += kFcfSize;

    // Sequence Number
    length += kDsnSize;

    // Destinatino PAN + Address
    switch (aFcf & Frame::kFcfDstAddrMask)
    {
    case Frame::kFcfDstAddrNone:
        break;

    case Frame::kFcfDstAddrShort:
        length += sizeof(PanId) + sizeof(ShortAddress);
        break;

    case Frame::kFcfDstAddrExt:
        length += sizeof(PanId) + sizeof(ExtAddress);
        break;

    default:
        assert(false);
    }

    // Source PAN + Address
    switch (aFcf & Frame::kFcfSrcAddrMask)
    {
    case Frame::kFcfSrcAddrNone:
        break;

    case Frame::kFcfSrcAddrShort:
        if ((aFcf & Frame::kFcfPanidCompression) == 0)
        {
            length += sizeof(PanId);
        }

        length += sizeof(ShortAddress);
        break;

    case Frame::kFcfSrcAddrExt:
        if ((aFcf & Frame::kFcfPanidCompression) == 0)
        {
            length += sizeof(PanId);
        }

        length += sizeof(ExtAddress);
        break;

    default:
        assert(false);
    }

    // Security Header
    if (aFcf & Frame::kFcfSecurityEnabled)
    {
        bytes[length] = aSecurityControl;

        if (aSecurityControl & kSecLevelMask)
        {
            length += 5;
        }

        switch (aSecurityControl & kKeyIdModeMask)
        {
        case kKeyIdMode0:
            length += kKeyIdLengthMode0;
            break;

        case kKeyIdMode1:
            length += kKeyIdLengthMode1;
            break;

        case kKeyIdMode2:
            length += kKeyIdLengthMode2;
            break;

        case kKeyIdMode3:
            length += kKeyIdLengthMode3;
            break;
        }
    }

    // Command ID
    if ((aFcf & kFcfFrameTypeMask) == kFcfFrameMacCmd)
    {
        length += kCommandIdSize;
    }

    SetPsduLength(length + GetFooterLength());

    return kThreadError_None;
}

uint8_t Frame::GetType(void)
{
    return GetPsdu()[0] & Frame::kFcfFrameTypeMask;
}

bool Frame::GetSecurityEnabled(void)
{
    return (GetPsdu()[0] & Frame::kFcfSecurityEnabled) != 0;
}

bool Frame::GetAckRequest(void)
{
    return (GetPsdu()[0] & Frame::kFcfAckRequest) != 0;
}

void Frame::SetAckRequest(bool aAckRequest)
{
    if (aAckRequest)
    {
        GetPsdu()[0] |= Frame::kFcfAckRequest;
    }
    else
    {
        GetPsdu()[0] &= ~Frame::kFcfAckRequest;
    }
}

bool Frame::GetFramePending(void)
{
    return (GetPsdu()[0] & Frame::kFcfFramePending) != 0;
}

void Frame::SetFramePending(bool aFramePending)
{
    if (aFramePending)
    {
        GetPsdu()[0] |= Frame::kFcfFramePending;
    }
    else
    {
        GetPsdu()[0] &= ~Frame::kFcfFramePending;
    }
}

uint8_t *Frame::FindSequence(void)
{
    uint8_t *cur = GetPsdu();

    // Frame Control Field
    cur += kFcfSize;

    return cur;
}

uint8_t Frame::GetSequence(void)
{
    uint8_t *buf = FindSequence();
    return buf[0];
}

void Frame::SetSequence(uint8_t aSequence)
{
    uint8_t *buf = FindSequence();
    buf[0] = aSequence;
}

uint8_t *Frame::FindDstPanId(void)
{
    uint8_t *cur = GetPsdu();
    uint16_t fcf = (static_cast<uint16_t>(GetPsdu()[1]) << 8) | GetPsdu()[0];

    VerifyOrExit((fcf & Frame::kFcfDstAddrMask) != Frame::kFcfDstAddrNone, cur = NULL);

    // Frame Control Field
    cur += kFcfSize;
    // Sequence Number
    cur += kDsnSize;

exit:
    return cur;
}

ThreadError Frame::GetDstPanId(PanId &aPanId)
{
    ThreadError error = kThreadError_None;
    uint8_t *buf;

    VerifyOrExit((buf = FindDstPanId()) != NULL, error = kThreadError_Parse);

    aPanId = (static_cast<uint16_t>(buf[1]) << 8) | buf[0];

exit:
    return error;
}

ThreadError Frame::SetDstPanId(PanId aPanId)
{
    uint8_t *buf;

    buf = FindDstPanId();
    assert(buf != NULL);

    buf[0] = aPanId;
    buf[1] = aPanId >> 8;

    return kThreadError_None;
}

uint8_t *Frame::FindDstAddr(void)
{
    uint8_t *cur = GetPsdu();

    // Frame Control Field
    cur += kFcfSize;
    // Sequence Number
    cur += kDsnSize;
    // Destination PAN
    cur += sizeof(PanId);

    return cur;
}

ThreadError Frame::GetDstAddr(Address &aAddress)
{
    ThreadError error = kThreadError_None;
    uint8_t *buf;
    uint16_t fcf = (static_cast<uint16_t>(GetPsdu()[1]) << 8) | GetPsdu()[0];

    VerifyOrExit(buf = FindDstAddr(), error = kThreadError_Parse);

    switch (fcf & Frame::kFcfDstAddrMask)
    {
    case Frame::kFcfDstAddrShort:
        aAddress.mLength = sizeof(ShortAddress);
        aAddress.mShortAddress = (static_cast<uint16_t>(buf[1]) << 8) | buf[0];
        break;

    case Frame::kFcfDstAddrExt:
        aAddress.mLength = sizeof(ExtAddress);

        for (unsigned int i = 0; i < sizeof(ExtAddress); i++)
        {
            aAddress.mExtAddress.m8[i] = buf[sizeof(ExtAddress) - 1 - i];
        }

        break;

    default:
        aAddress.mLength = 0;
        break;
    }

exit:
    return error;
}

ThreadError Frame::SetDstAddr(ShortAddress aShortAddress)
{
    uint8_t *buf;
    uint16_t fcf = (static_cast<uint16_t>(GetPsdu()[1]) << 8) | GetPsdu()[0];

    assert((fcf & Frame::kFcfDstAddrMask) == Frame::kFcfDstAddrShort);

    buf = FindDstAddr();
    assert(buf != NULL);

    buf[0] = aShortAddress;
    buf[1] = aShortAddress >> 8;

    return kThreadError_None;
}

ThreadError Frame::SetDstAddr(const ExtAddress &aExtAddress)
{
    uint8_t *buf;
    uint16_t fcf = (static_cast<uint16_t>(GetPsdu()[1]) << 8) | GetPsdu()[0];

    assert((fcf & Frame::kFcfDstAddrMask) == Frame::kFcfDstAddrExt);

    buf = FindDstAddr();
    assert(buf != NULL);

    for (unsigned int i = 0; i < sizeof(ExtAddress); i++)
    {
        buf[i] = aExtAddress.m8[sizeof(ExtAddress) - 1 - i];
    }

    return kThreadError_None;
}

uint8_t *Frame::FindSrcPanId(void)
{
    uint8_t *cur = GetPsdu();
    uint16_t fcf = (static_cast<uint16_t>(GetPsdu()[1]) << 8) | GetPsdu()[0];

    VerifyOrExit((fcf & Frame::kFcfDstAddrMask) != Frame::kFcfDstAddrNone ||
                 (fcf & Frame::kFcfSrcAddrMask) != Frame::kFcfSrcAddrNone, cur = NULL);

    // Frame Control Field
    cur += kFcfSize;
    // Sequence Number
    cur += kDsnSize;

    if ((fcf & Frame::kFcfPanidCompression) == 0)
    {
        // Destination PAN + Address
        switch (fcf & Frame::kFcfDstAddrMask)
        {
        case Frame::kFcfDstAddrShort:
            cur += sizeof(PanId) + sizeof(ShortAddress);
            break;

        case Frame::kFcfDstAddrExt:
            cur += sizeof(PanId) + sizeof(ExtAddress);
            break;
        }
    }

exit:
    return cur;
}

ThreadError Frame::GetSrcPanId(PanId &aPanId)
{
    ThreadError error = kThreadError_None;
    uint8_t *buf;

    VerifyOrExit((buf = FindSrcPanId()) != NULL, error = kThreadError_Parse);

    aPanId = (static_cast<uint16_t>(buf[1]) << 8) | buf[0];

exit:
    return error;
}

ThreadError Frame::SetSrcPanId(PanId aPanId)
{
    ThreadError error = kThreadError_None;
    uint8_t *buf;

    VerifyOrExit((buf = FindSrcPanId()) != NULL, error = kThreadError_Parse);

    buf[0] = aPanId;
    buf[1] = aPanId >> 8;

exit:
    return error;
}

uint8_t *Frame::FindSrcAddr(void)
{
    uint8_t *cur = GetPsdu();
    uint16_t fcf = (static_cast<uint16_t>(GetPsdu()[1]) << 8) | GetPsdu()[0];

    // Frame Control Field
    cur += kFcfSize;
    // Sequence Number
    cur += kDsnSize;

    // Destination PAN + Address
    switch (fcf & Frame::kFcfDstAddrMask)
    {
    case Frame::kFcfDstAddrShort:
        cur += sizeof(PanId) + sizeof(ShortAddress);
        break;

    case Frame::kFcfDstAddrExt:
        cur += sizeof(PanId) + sizeof(ExtAddress);
        break;
    }

    // Source PAN
    if ((fcf & Frame::kFcfPanidCompression) == 0)
    {
        cur += sizeof(PanId);
    }

    return cur;
}

ThreadError Frame::GetSrcAddr(Address &address)
{
    ThreadError error = kThreadError_None;
    uint8_t *buf;
    uint16_t fcf = (static_cast<uint16_t>(GetPsdu()[1]) << 8) | GetPsdu()[0];

    VerifyOrExit((buf = FindSrcAddr()) != NULL, error = kThreadError_Parse);

    switch (fcf & Frame::kFcfSrcAddrMask)
    {
    case Frame::kFcfSrcAddrShort:
        address.mLength = sizeof(ShortAddress);
        address.mShortAddress = (static_cast<uint16_t>(buf[1]) << 8) | buf[0];;
        break;

    case Frame::kFcfSrcAddrExt:
        address.mLength = sizeof(ExtAddress);

        for (unsigned int i = 0; i < sizeof(ExtAddress); i++)
        {
            address.mExtAddress.m8[i] = buf[sizeof(ExtAddress) - 1 - i];
        }

        break;

    default:
        address.mLength = 0;
        break;
    }

exit:
    return error;
}

ThreadError Frame::SetSrcAddr(ShortAddress aShortAddress)
{
    uint8_t *buf;
    uint16_t fcf = (static_cast<uint16_t>(GetPsdu()[1]) << 8) | GetPsdu()[0];

    assert((fcf & Frame::kFcfSrcAddrMask) == Frame::kFcfSrcAddrShort);

    buf = FindSrcAddr();
    assert(buf != NULL);

    buf[0] = aShortAddress;
    buf[1] = aShortAddress >> 8;

    return kThreadError_None;
}

ThreadError Frame::SetSrcAddr(const ExtAddress &aExtAddress)
{
    uint8_t *buf;
    uint16_t fcf = (static_cast<uint16_t>(GetPsdu()[1]) << 8) | GetPsdu()[0];

    assert((fcf & Frame::kFcfSrcAddrMask) == Frame::kFcfSrcAddrExt);

    buf = FindSrcAddr();
    assert(buf != NULL);

    for (unsigned int i = 0; i < sizeof(aExtAddress); i++)
    {
        buf[i] = aExtAddress.m8[sizeof(aExtAddress) - 1 - i];
    }

    return kThreadError_None;
}

uint8_t *Frame::FindSecurityHeader(void)
{
    uint8_t *cur = GetPsdu();
    uint16_t fcf = (static_cast<uint16_t>(GetPsdu()[1]) << 8) | GetPsdu()[0];

    VerifyOrExit((fcf & Frame::kFcfSecurityEnabled) != 0, cur = NULL);

    // Frame Control Field
    cur += kFcfSize;
    // Sequence Number
    cur += kDsnSize;

    // Destination PAN + Address
    switch (fcf & Frame::kFcfDstAddrMask)
    {
    case Frame::kFcfDstAddrShort:
        cur += sizeof(PanId) + sizeof(ShortAddress);
        break;

    case Frame::kFcfDstAddrExt:
        cur += sizeof(PanId) + sizeof(ExtAddress);
        break;
    }

    // Source PAN + Address
    switch (fcf & Frame::kFcfSrcAddrMask)
    {
    case Frame::kFcfSrcAddrShort:
        if ((fcf & Frame::kFcfPanidCompression) == 0)
        {
            cur += sizeof(PanId);
        }

        cur += sizeof(ShortAddress);
        break;

    case Frame::kFcfSrcAddrExt:
        if ((fcf & Frame::kFcfPanidCompression) == 0)
        {
            cur += sizeof(PanId);
        }

        cur += sizeof(ExtAddress);
        break;
    }

exit:
    return cur;
}

ThreadError Frame::GetSecurityLevel(uint8_t &aSecurityLevel)
{
    ThreadError error = kThreadError_None;
    uint8_t *buf;

    VerifyOrExit((buf = FindSecurityHeader()) != NULL, error = kThreadError_Parse);

    aSecurityLevel = buf[0] & kSecLevelMask;

exit:
    return error;
}

ThreadError Frame::GetFrameCounter(uint32_t &aFrameCounter)
{
    ThreadError error = kThreadError_None;
    uint8_t *buf;

    VerifyOrExit((buf = FindSecurityHeader()) != NULL, error = kThreadError_Parse);

    // Security Control
    buf += kSecurityControlSize;

    aFrameCounter = ((static_cast<uint32_t>(buf[3]) << 24) |
                     (static_cast<uint32_t>(buf[2]) << 16) |
                     (static_cast<uint32_t>(buf[1]) << 8) |
                     (static_cast<uint32_t>(buf[0])));

exit:
    return error;
}

ThreadError Frame::SetFrameCounter(uint32_t aFrameCounter)
{
    uint8_t *buf;

    buf = FindSecurityHeader();
    assert(buf != NULL);

    // Security Control
    buf += kSecurityControlSize;

    buf[0] = aFrameCounter;
    buf[1] = aFrameCounter >> 8;
    buf[2] = aFrameCounter >> 16;
    buf[3] = aFrameCounter >> 24;

    return kThreadError_None;
}

ThreadError Frame::GetKeyId(uint8_t &aKeyId)
{
    ThreadError error = kThreadError_None;
    uint8_t *buf;

    VerifyOrExit((buf = FindSecurityHeader()) != NULL, error = kThreadError_Parse);

    // Security Control + Frame Counter
    buf += kSecurityControlSize + kFrameCounterSize;

    aKeyId = buf[0];

exit:
    return error;
}

ThreadError Frame::SetKeyId(uint8_t aKeyId)
{
    uint8_t *buf;

    buf = FindSecurityHeader();
    assert(buf != NULL);

    // Security Control + Frame Counter
    buf += kSecurityControlSize + kFrameCounterSize;

    buf[0] = aKeyId;

    return kThreadError_None;
}

ThreadError Frame::GetCommandId(uint8_t &aCommandId)
{
    ThreadError error = kThreadError_None;
    uint8_t *buf;

    VerifyOrExit((buf = GetPayload()) != NULL, error = kThreadError_Parse);
    aCommandId = buf[-1];

exit:
    return error;
}

ThreadError Frame::SetCommandId(uint8_t aCommandId)
{
    ThreadError error = kThreadError_None;
    uint8_t *buf;

    VerifyOrExit((buf = GetPayload()) != NULL, error = kThreadError_Parse);
    buf[-1] = aCommandId;

exit:
    return error;
}

uint8_t Frame::GetLength() const
{
    return GetPsduLength();
}

ThreadError Frame::SetLength(uint8_t aLength)
{
    SetPsduLength(aLength);
    return kThreadError_None;
}

uint8_t Frame::GetHeaderLength(void)
{
    return GetPayload() - GetPsdu();
}

uint8_t Frame::GetFooterLength(void)
{
    uint8_t footerLength = 0;
    uint8_t *cur;

    VerifyOrExit((cur = FindSecurityHeader()) != NULL, ;);

    switch (cur[0] & kSecLevelMask)
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

uint8_t Frame::GetMaxPayloadLength(void)
{
    return kMTU - (GetHeaderLength() + GetFooterLength());
}

uint8_t Frame::GetPayloadLength(void)
{
    return GetPsduLength() - (GetHeaderLength() + GetFooterLength());
}

ThreadError Frame::SetPayloadLength(uint8_t aLength)
{
    SetPsduLength(GetHeaderLength() + GetFooterLength() + aLength);
    return kThreadError_None;
}

uint8_t *Frame::GetHeader(void)
{
    return GetPsdu();
}

uint8_t *Frame::GetPayload(void)
{
    uint8_t *cur = GetPsdu();
    uint16_t fcf = (static_cast<uint16_t>(GetPsdu()[1]) << 8) | GetPsdu()[0];
    uint8_t securityControl;

    // Frame Control
    cur += kFcfSize;
    // Sequence Number
    cur += kDsnSize;

    // Destination PAN + Address
    switch (fcf & Frame::kFcfDstAddrMask)
    {
    case Frame::kFcfDstAddrNone:
        break;

    case Frame::kFcfDstAddrShort:
        cur += sizeof(PanId) + sizeof(ShortAddress);
        break;

    case Frame::kFcfDstAddrExt:
        cur += sizeof(PanId) + sizeof(ExtAddress);
        break;

    default:
        ExitNow(cur = NULL);
    }

    // Source PAN + Address
    switch (fcf & Frame::kFcfSrcAddrMask)
    {
    case Frame::kFcfSrcAddrNone:
        break;

    case Frame::kFcfSrcAddrShort:
        if ((fcf & Frame::kFcfPanidCompression) == 0)
        {
            cur += sizeof(PanId);
        }

        cur += sizeof(ShortAddress);
        break;

    case Frame::kFcfSrcAddrExt:
        if ((fcf & Frame::kFcfPanidCompression) == 0)
        {
            cur += sizeof(PanId);
        }

        cur += sizeof(ExtAddress);
        break;

    default:
        ExitNow(cur = NULL);
    }

    // Security Control + Frame Counter + Key Identifier
    if ((fcf & Frame::kFcfSecurityEnabled) != 0)
    {
        securityControl = *cur;

        if (securityControl & kSecLevelMask)
        {
            cur += kSecurityControlSize + kFrameCounterSize;
        }

        switch (securityControl & kKeyIdModeMask)
        {
        case kKeyIdMode0:
            cur += kKeyIdLengthMode0;
            break;

        case kKeyIdMode1:
            cur += kKeyIdLengthMode1;
            break;

        case kKeyIdMode2:
            cur += kKeyIdLengthMode2;
            break;

        case kKeyIdMode3:
            cur += kKeyIdLengthMode3;
            break;
        }
    }

    // Command ID
    if ((fcf & kFcfFrameTypeMask) == kFcfFrameMacCmd)
    {
        cur += kCommandIdSize;
    }

exit:
    return cur;
}

uint8_t *Frame::GetFooter(void)
{
    return GetPsdu() + GetPsduLength() - GetFooterLength();
}

}  // namespace Mac
}  // namespace Thread

