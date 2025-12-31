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
 *   This file implements common methods for manipulating MLE TLVs.
 */

#include "tlvs.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/message.hpp"

namespace ot {

uint32_t Tlv::GetSize(void) const
{
    return IsExtended() ? sizeof(ExtendedTlv) + As<ExtendedTlv>(this)->GetLength() : sizeof(Tlv) + GetLength();
}

uint8_t *Tlv::GetValue(void)
{
    return reinterpret_cast<uint8_t *>(this) + (IsExtended() ? sizeof(ExtendedTlv) : sizeof(Tlv));
}

const uint8_t *Tlv::GetValue(void) const
{
    return reinterpret_cast<const uint8_t *>(this) + (IsExtended() ? sizeof(ExtendedTlv) : sizeof(Tlv));
}

Error Tlv::AppendTo(Message &aMessage) const { return aMessage.AppendBytes(this, static_cast<uint16_t>(GetSize())); }

Error Tlv::FindTlv(const Message &aMessage, uint8_t aType, uint16_t aMaxSize, Tlv &aTlv)
{
    uint16_t offset;

    return FindTlv(aMessage, aType, aMaxSize, aTlv, offset);
}

Error Tlv::FindTlv(const Message &aMessage, uint8_t aType, uint16_t aMaxSize, Tlv &aTlv, uint16_t &aOffset)
{
    Error      error;
    ParsedInfo info;

    SuccessOrExit(error = info.FindIn(aMessage, aType));

    info.mTlvOffsetRange.ShrinkLength(aMaxSize);
    aMessage.ReadBytes(info.mTlvOffsetRange, &aTlv);
    aOffset = info.mTlvOffsetRange.GetOffset();

exit:
    return error;
}

Error Tlv::FindTlvValueOffsetRange(const Message &aMessage, uint8_t aType, OffsetRange &aOffsetRange)
{
    Error      error;
    ParsedInfo info;

    SuccessOrExit(error = info.FindIn(aMessage, aType));
    aOffsetRange = info.mValueOffsetRange;

exit:
    return error;
}

Error Tlv::ParsedInfo::ParseFrom(const Message &aMessage, uint16_t aOffset)
{
    OffsetRange offsetRange;

    offsetRange.InitFromRange(aOffset, aMessage.GetLength());
    return ParseFrom(aMessage, offsetRange);
}

Error Tlv::ParsedInfo::ParseFrom(const Message &aMessage, const OffsetRange &aOffsetRange)
{
    Error       error;
    Tlv         tlv;
    ExtendedTlv extTlv;
    uint32_t    headerSize;
    uint32_t    size;

    SuccessOrExit(error = aMessage.Read(aOffsetRange, tlv));

    mType = tlv.GetType();

    if (!tlv.IsExtended())
    {
        mIsExtended = false;
        headerSize  = sizeof(Tlv);
        size        = headerSize + tlv.GetLength();
    }
    else
    {
        SuccessOrExit(error = aMessage.Read(aOffsetRange, extTlv));

        mIsExtended = true;
        headerSize  = sizeof(ExtendedTlv);
        size        = headerSize + extTlv.GetLength();
    }

    mTlvOffsetRange = aOffsetRange;
    VerifyOrExit(mTlvOffsetRange.Contains(size), error = kErrorParse);
    mTlvOffsetRange.ShrinkLength(static_cast<uint16_t>(size));

    VerifyOrExit(mTlvOffsetRange.GetEndOffset() <= aMessage.GetLength(), error = kErrorParse);

    mValueOffsetRange = mTlvOffsetRange;
    mValueOffsetRange.AdvanceOffset(headerSize);

exit:
    return error;
}

Error Tlv::ParsedInfo::FindIn(const Message &aMessage, uint8_t aType)
{
    Error       error = kErrorNotFound;
    OffsetRange offsetRange;

    offsetRange.InitFromMessageOffsetToEnd(aMessage);

    while (true)
    {
        SuccessOrExit(ParseFrom(aMessage, offsetRange));

        if (mType == aType)
        {
            error = kErrorNone;
            ExitNow();
        }

        offsetRange.AdvanceOffset(mTlvOffsetRange.GetLength());
    }

exit:
    return error;
}

Error Tlv::ReadStringTlv(const Message &aMessage, uint16_t aOffset, uint8_t aMaxStringLength, char *aValue)
{
    Error      error = kErrorNone;
    ParsedInfo info;

    SuccessOrExit(error = info.ParseFrom(aMessage, aOffset));

    info.mValueOffsetRange.ShrinkLength(aMaxStringLength);
    aMessage.ReadBytes(info.mValueOffsetRange, aValue);
    aValue[info.mValueOffsetRange.GetLength()] = kNullChar;

exit:
    return error;
}

template <typename UintType> Error Tlv::ReadUintTlv(const Message &aMessage, uint16_t aOffset, UintType &aValue)
{
    Error error;

    SuccessOrExit(error = ReadTlvValue(aMessage, aOffset, &aValue, sizeof(aValue)));
    aValue = BigEndian::HostSwap<UintType>(aValue);

exit:
    return error;
}

// Explicit instantiations of `ReadUintTlv<>()`
template Error Tlv::ReadUintTlv<uint8_t>(const Message &aMessage, uint16_t aOffset, uint8_t &aValue);
template Error Tlv::ReadUintTlv<uint16_t>(const Message &aMessage, uint16_t aOffset, uint16_t &aValue);
template Error Tlv::ReadUintTlv<uint32_t>(const Message &aMessage, uint16_t aOffset, uint32_t &aValue);

Error Tlv::ReadTlvValue(const Message &aMessage, uint16_t aOffset, void *aValue, uint8_t aMinLength)
{
    Error      error;
    ParsedInfo info;

    SuccessOrExit(error = info.ParseFrom(aMessage, aOffset));

    VerifyOrExit(info.mValueOffsetRange.Contains(aMinLength), error = kErrorParse);
    info.mValueOffsetRange.ShrinkLength(aMinLength);

    aMessage.ReadBytes(info.mValueOffsetRange, aValue);

exit:
    return error;
}

Error Tlv::FindStringTlv(const Message &aMessage, uint8_t aType, uint8_t aMaxStringLength, char *aValue)
{
    Error      error;
    ParsedInfo info;

    SuccessOrExit(error = info.FindIn(aMessage, aType));
    error = ReadStringTlv(aMessage, info.mTlvOffsetRange.GetOffset(), aMaxStringLength, aValue);

exit:
    return error;
}

template <typename UintType> Error Tlv::FindUintTlv(const Message &aMessage, uint8_t aType, UintType &aValue)
{
    Error      error;
    ParsedInfo info;

    SuccessOrExit(error = info.FindIn(aMessage, aType));
    error = ReadUintTlv<UintType>(aMessage, info.mTlvOffsetRange.GetOffset(), aValue);

exit:
    return error;
}

// Explicit instantiations of `FindUintTlv<>()`
template Error Tlv::FindUintTlv<uint8_t>(const Message &aMessage, uint8_t aType, uint8_t &aValue);
template Error Tlv::FindUintTlv<uint16_t>(const Message &aMessage, uint8_t aType, uint16_t &aValue);
template Error Tlv::FindUintTlv<uint32_t>(const Message &aMessage, uint8_t aType, uint32_t &aValue);

Error Tlv::FindTlv(const Message &aMessage, uint8_t aType, void *aValue, uint16_t aLength)
{
    Error       error;
    OffsetRange offsetRange;

    SuccessOrExit(error = FindTlvValueOffsetRange(aMessage, aType, offsetRange));
    error = aMessage.Read(offsetRange, aValue, aLength);

exit:
    return error;
}

Error Tlv::AppendStringTlv(Message &aMessage, uint8_t aType, uint8_t aMaxStringLength, const char *aValue)
{
    uint16_t length = (aValue == nullptr) ? 0 : StringLength(aValue, aMaxStringLength);

    return AppendTlv(aMessage, aType, aValue, static_cast<uint8_t>(length));
}

Error Tlv::ValidateStringTlvValue(uint8_t aMaxStringLength, const char *aStringValue)
{
    Error error = kErrorNone;

    VerifyOrExit(aStringValue != nullptr);

    VerifyOrExit(StringLength(aStringValue, aMaxStringLength + 1) <= aMaxStringLength, error = kErrorInvalidArgs);
    VerifyOrExit(IsValidUtf8String(aStringValue), error = kErrorInvalidArgs);

exit:
    return error;
}

template <typename UintType> Error Tlv::AppendUintTlv(Message &aMessage, uint8_t aType, UintType aValue)
{
    UintType value = BigEndian::HostSwap<UintType>(aValue);

    return AppendTlv(aMessage, aType, &value, sizeof(UintType));
}

// Explicit instantiations of `AppendUintTlv<>()`
template Error Tlv::AppendUintTlv<uint8_t>(Message &aMessage, uint8_t aType, uint8_t aValue);
template Error Tlv::AppendUintTlv<uint16_t>(Message &aMessage, uint8_t aType, uint16_t aValue);
template Error Tlv::AppendUintTlv<uint32_t>(Message &aMessage, uint8_t aType, uint32_t aValue);

Error Tlv::AppendEmptyTlv(Message &aMessage, uint8_t aType) { return AppendTlv(aMessage, aType, nullptr, 0); }

Error Tlv::AppendTlv(Message &aMessage, uint8_t aType, const void *aValue, uint16_t aLength)
{
    Error       error = kErrorNone;
    ExtendedTlv extTlv;
    Tlv         tlv;

    if (aLength > kBaseTlvMaxLength)
    {
        extTlv.SetType(aType);
        extTlv.SetLength(aLength);
        SuccessOrExit(error = aMessage.Append(extTlv));
    }
    else
    {
        tlv.SetType(aType);
        tlv.SetLength(static_cast<uint8_t>(aLength));
        SuccessOrExit(error = aMessage.Append(tlv));
    }

    VerifyOrExit(aLength > 0);
    error = aMessage.AppendBytes(aValue, aLength);

exit:
    return error;
}

Error Tlv::StartTlv(Message &aMessage, uint8_t aType, Bookmark &aBookmark)
{
    Tlv tlv;

    tlv.SetType(aType);
    tlv.SetLength(0);

    aBookmark = aMessage.GetLength();

    return aMessage.Append(tlv);
}

Error Tlv::AdjustTlv(Message &aMessage, Bookmark aBookmark)
{
    return UpdateTlv(aMessage, aBookmark, /* aShouldWriteLength */ false);
}

Error Tlv::EndTlv(Message &aMessage, Bookmark aBookmark)
{
    return UpdateTlv(aMessage, aBookmark, /* aShouldWriteLength */ true);
}

Error Tlv::UpdateTlv(Message &aMessage, Bookmark aBookmark, bool aShouldWriteLength)
{
    Error       error;
    uint16_t    startOffset = aBookmark;
    uint16_t    length;
    Tlv         tlv;
    ExtendedTlv extTlv;

    SuccessOrExit(error = aMessage.Read(startOffset, tlv));

    length = aMessage.GetLength() - startOffset;

    if (tlv.IsExtended())
    {
        length -= sizeof(ExtendedTlv);
    }
    else
    {
        length -= sizeof(Tlv);

        if (length > kBaseTlvMaxLength)
        {
            // If the TLV is not already an Extended TLV, change it. We
            // need to move the written value bytes forward to make
            // room for the Extended TLV header.

            SuccessOrExit(error = aMessage.SetLength(aMessage.GetLength() + sizeof(ExtendedTlv) - sizeof(Tlv)));

            aMessage.WriteBytesFromMessage(/* aWriteOffset */ startOffset + sizeof(ExtendedTlv), aMessage,
                                           /* aReadOffset */ startOffset + sizeof(Tlv), length);

            tlv.SetLength(kExtendedLength);
            aMessage.Write(startOffset, tlv);
        }
    }

    VerifyOrExit(aShouldWriteLength);

    if (!tlv.IsExtended())
    {
        tlv.SetLength(static_cast<uint8_t>(length));
        aMessage.Write(startOffset, tlv);
    }
    else
    {
        extTlv.SetType(tlv.GetType());
        extTlv.SetLength(length);
        aMessage.Write(startOffset, extTlv);
    }

exit:
    return error;
}

const Tlv *Tlv::FindTlv(const void *aTlvsStart, uint16_t aTlvsLength, uint8_t aType)
{
    const Tlv *tlv;
    const Tlv *end = reinterpret_cast<const Tlv *>(reinterpret_cast<const uint8_t *>(aTlvsStart) + aTlvsLength);

    for (tlv = reinterpret_cast<const Tlv *>(aTlvsStart); tlv < end; tlv = tlv->GetNext())
    {
        VerifyOrExit((tlv + 1) <= end, tlv = nullptr);

        if (tlv->IsExtended())
        {
            VerifyOrExit((As<ExtendedTlv>(tlv) + 1) <= As<ExtendedTlv>(end), tlv = nullptr);
        }

        VerifyOrExit(tlv->GetNext() <= end, tlv = nullptr);

        if (tlv->GetType() == aType)
        {
            ExitNow();
        }
    }

    tlv = nullptr;

exit:
    return tlv;
}

} // namespace ot
