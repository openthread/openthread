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

    if (aMaxSize > info.mSize)
    {
        aMaxSize = info.mSize;
    }

    aMessage.ReadBytes(info.mOffset, &aTlv, aMaxSize);
    aOffset = info.mOffset;

exit:
    return error;
}
Error Tlv::FindTlvValueOffset(const Message &aMessage, uint8_t aType, uint16_t &aValueOffset, uint16_t &aLength)
{
    Error      error;
    ParsedInfo info;

    SuccessOrExit(error = info.FindIn(aMessage, aType));

    aValueOffset = info.mValueOffset;
    aLength      = info.mLength;

exit:
    return error;
}

Error Tlv::FindTlvValueStartEndOffsets(const Message &aMessage,
                                       uint8_t        aType,
                                       uint16_t      &aValueStartOffset,
                                       uint16_t      &aValueEndOffset)
{
    Error      error;
    ParsedInfo info;

    SuccessOrExit(error = info.FindIn(aMessage, aType));

    aValueStartOffset = info.mValueOffset;
    aValueEndOffset   = info.mValueOffset + info.mLength;

exit:
    return error;
}

Error Tlv::ParsedInfo::ParseFrom(const Message &aMessage, uint16_t aOffset)
{
    // This method reads and parses the TLV info from `aMessage` at
    // `aOffset`. This can be used independent of whether the TLV is
    // extended or not. It validates that the entire TLV can be read
    // from `aMessage`.  Returns `kErrorNone` when successfully parsed,
    // otherwise `kErrorParse`.

    Error       error;
    Tlv         tlv;
    ExtendedTlv extTlv;
    uint16_t    headerSize;

    SuccessOrExit(error = aMessage.Read(aOffset, tlv));

    if (!tlv.IsExtended())
    {
        mType      = tlv.GetType();
        mLength    = tlv.GetLength();
        headerSize = sizeof(Tlv);
    }
    else
    {
        SuccessOrExit(error = aMessage.Read(aOffset, extTlv));

        mType      = extTlv.GetType();
        mLength    = extTlv.GetLength();
        headerSize = sizeof(ExtendedTlv);
    }

    // We know that we could successfully read `tlv` or `extTlv`
    // (`headerSize` bytes) from the message, so the calculation of the
    // remaining length as `aMessage.GetLength() - aOffset - headerSize`
    // cannot underflow.

    VerifyOrExit(mLength <= aMessage.GetLength() - aOffset - headerSize, error = kErrorParse);

    // Now that we know the entire TLV is contained within the
    // `aMessage`, we can safely calculate `mValueOffset` and `mSize`
    // as `uint16_t` and know that there will be no overflow.

    mType        = tlv.GetType();
    mOffset      = aOffset;
    mValueOffset = aOffset + headerSize;
    mSize        = mLength + headerSize;

exit:
    return error;
}

Error Tlv::ParsedInfo::FindIn(const Message &aMessage, uint8_t aType)
{
    // This  method searches within `aMessage` starting from
    // `aMessage.GetOffset()` for a TLV type `aType` and parsed its
    // info and validates that the entire TLV can be read from
    // `aMessage`. Returns `kErrorNone` when found, otherwise
    // `kErrorNotFound`.

    Error    error  = kErrorNotFound;
    uint16_t offset = aMessage.GetOffset();

    while (true)
    {
        SuccessOrExit(ParseFrom(aMessage, offset));

        if (mType == aType)
        {
            error = kErrorNone;
            ExitNow();
        }

        // `ParseFrom()` already validated that `offset + mSize` is
        // less than `aMessage.GetLength()` and therefore we can not
        // have an overflow here.

        offset += mSize;
    }

exit:
    return error;
}

Error Tlv::ReadStringTlv(const Message &aMessage, uint16_t aOffset, uint8_t aMaxStringLength, char *aValue)
{
    Error      error = kErrorNone;
    ParsedInfo info;
    uint16_t   length;

    SuccessOrExit(error = info.ParseFrom(aMessage, aOffset));

    length = Min(info.mLength, static_cast<uint16_t>(aMaxStringLength));

    aMessage.ReadBytes(info.mValueOffset, aValue, length);
    aValue[length] = '\0';

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

    VerifyOrExit(info.mLength >= aMinLength, error = kErrorParse);

    aMessage.ReadBytes(info.mValueOffset, aValue, aMinLength);

exit:
    return error;
}

Error Tlv::FindStringTlv(const Message &aMessage, uint8_t aType, uint8_t aMaxStringLength, char *aValue)
{
    Error      error;
    ParsedInfo info;

    SuccessOrExit(error = info.FindIn(aMessage, aType));
    error = ReadStringTlv(aMessage, info.mOffset, aMaxStringLength, aValue);

exit:
    return error;
}

template <typename UintType> Error Tlv::FindUintTlv(const Message &aMessage, uint8_t aType, UintType &aValue)
{
    Error      error;
    ParsedInfo info;

    SuccessOrExit(error = info.FindIn(aMessage, aType));
    error = ReadUintTlv<UintType>(aMessage, info.mOffset, aValue);

exit:
    return error;
}

// Explicit instantiations of `FindUintTlv<>()`
template Error Tlv::FindUintTlv<uint8_t>(const Message &aMessage, uint8_t aType, uint8_t &aValue);
template Error Tlv::FindUintTlv<uint16_t>(const Message &aMessage, uint8_t aType, uint16_t &aValue);
template Error Tlv::FindUintTlv<uint32_t>(const Message &aMessage, uint8_t aType, uint32_t &aValue);

Error Tlv::FindTlv(const Message &aMessage, uint8_t aType, void *aValue, uint16_t aLength)
{
    Error    error;
    uint16_t offset;
    uint16_t length;

    SuccessOrExit(error = FindTlvValueOffset(aMessage, aType, offset, length));
    VerifyOrExit(length >= aLength, error = kErrorParse);
    aMessage.ReadBytes(offset, aValue, aLength);

exit:
    return error;
}

Error Tlv::AppendStringTlv(Message &aMessage, uint8_t aType, uint8_t aMaxStringLength, const char *aValue)
{
    uint16_t length = (aValue == nullptr) ? 0 : StringLength(aValue, aMaxStringLength);

    return AppendTlv(aMessage, aType, aValue, static_cast<uint8_t>(length));
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

Error Tlv::AppendTlv(Message &aMessage, uint8_t aType, const void *aValue, uint8_t aLength)
{
    Error error = kErrorNone;
    Tlv   tlv;

    OT_ASSERT(aLength <= Tlv::kBaseTlvMaxLength);

    tlv.SetType(aType);
    tlv.SetLength(aLength);
    SuccessOrExit(error = aMessage.Append(tlv));

    VerifyOrExit(aLength > 0);
    error = aMessage.AppendBytes(aValue, aLength);

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
