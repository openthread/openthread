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
    return IsExtended() ? sizeof(ExtendedTlv) + static_cast<const ExtendedTlv *>(this)->GetLength()
                        : sizeof(Tlv) + GetLength();
}

uint8_t *Tlv::GetValue(void)
{
    return reinterpret_cast<uint8_t *>(this) + (IsExtended() ? sizeof(ExtendedTlv) : sizeof(Tlv));
}

const uint8_t *Tlv::GetValue(void) const
{
    return reinterpret_cast<const uint8_t *>(this) + (IsExtended() ? sizeof(ExtendedTlv) : sizeof(Tlv));
}

otError Tlv::AppendTo(Message &aMessage) const
{
    return aMessage.AppendBytes(this, static_cast<uint16_t>(GetSize()));
}

otError Tlv::FindTlv(const Message &aMessage, uint8_t aType, uint16_t aMaxSize, Tlv &aTlv)
{
    otError  error;
    uint16_t offset;
    uint16_t size;

    SuccessOrExit(error = Find(aMessage, aType, &offset, &size, nullptr));

    if (aMaxSize > size)
    {
        aMaxSize = size;
    }

    aMessage.ReadBytes(offset, &aTlv, aMaxSize);

exit:
    return error;
}

otError Tlv::FindTlvOffset(const Message &aMessage, uint8_t aType, uint16_t &aOffset)
{
    return Find(aMessage, aType, &aOffset, nullptr, nullptr);
}

otError Tlv::FindTlvValueOffset(const Message &aMessage, uint8_t aType, uint16_t &aValueOffset, uint16_t &aLength)
{
    otError  error;
    uint16_t offset;
    uint16_t size;
    bool     isExtendedTlv;

    SuccessOrExit(error = Find(aMessage, aType, &offset, &size, &isExtendedTlv));

    if (!isExtendedTlv)
    {
        aValueOffset = offset + sizeof(Tlv);
        aLength      = size - sizeof(Tlv);
    }
    else
    {
        aValueOffset = offset + sizeof(ExtendedTlv);
        aLength      = size - sizeof(ExtendedTlv);
    }

exit:
    return error;
}

otError Tlv::Find(const Message &aMessage, uint8_t aType, uint16_t *aOffset, uint16_t *aSize, bool *aIsExtendedTlv)
{
    otError  error        = OT_ERROR_NOT_FOUND;
    uint16_t offset       = aMessage.GetOffset();
    uint16_t remainingLen = aMessage.GetLength();
    Tlv      tlv;
    uint32_t size;

    VerifyOrExit(offset <= remainingLen);
    remainingLen -= offset;

    while (true)
    {
        SuccessOrExit(aMessage.Read(offset, tlv));

        if (tlv.mLength != kExtendedLength)
        {
            size = tlv.GetSize();
        }
        else
        {
            ExtendedTlv extTlv;

            SuccessOrExit(aMessage.Read(offset, extTlv));

            VerifyOrExit(extTlv.GetLength() <= (remainingLen - sizeof(ExtendedTlv)));
            size = extTlv.GetSize();
        }

        VerifyOrExit(size <= remainingLen);

        if (tlv.GetType() == aType)
        {
            if (aOffset != nullptr)
            {
                *aOffset = offset;
            }

            if (aSize != nullptr)
            {
                *aSize = static_cast<uint16_t>(size);
            }

            if (aIsExtendedTlv != nullptr)
            {
                *aIsExtendedTlv = (tlv.mLength == kExtendedLength);
            }

            error = OT_ERROR_NONE;
            ExitNow();
        }

        offset += size;
        remainingLen -= size;
    }

exit:
    return error;
}

template <typename UintType> otError Tlv::ReadUintTlv(const Message &aMessage, uint16_t aOffset, UintType &aValue)
{
    otError error;

    SuccessOrExit(error = ReadTlv(aMessage, aOffset, &aValue, sizeof(aValue)));
    aValue = Encoding::BigEndian::HostSwap<UintType>(aValue);

exit:
    return error;
}

// Explicit instantiations of `ReadUintTlv<>()`
template otError Tlv::ReadUintTlv<uint8_t>(const Message &aMessage, uint16_t aOffset, uint8_t &aValue);
template otError Tlv::ReadUintTlv<uint16_t>(const Message &aMessage, uint16_t aOffset, uint16_t &aValue);
template otError Tlv::ReadUintTlv<uint32_t>(const Message &aMessage, uint16_t aOffset, uint32_t &aValue);

otError Tlv::ReadTlv(const Message &aMessage, uint16_t aOffset, void *aValue, uint8_t aMinLength)
{
    otError error = OT_ERROR_NONE;
    Tlv     tlv;

    SuccessOrExit(error = aMessage.Read(aOffset, tlv));
    VerifyOrExit(!tlv.IsExtended() && (tlv.GetLength() >= aMinLength), error = OT_ERROR_PARSE);
    VerifyOrExit(tlv.GetSize() + aOffset <= aMessage.GetLength(), error = OT_ERROR_PARSE);

    aMessage.ReadBytes(aOffset + sizeof(Tlv), aValue, aMinLength);

exit:
    return error;
}

template <typename UintType> otError Tlv::FindUintTlv(const Message &aMessage, uint8_t aType, UintType &aValue)
{
    otError  error = OT_ERROR_NONE;
    uint16_t offset;

    SuccessOrExit(error = FindTlvOffset(aMessage, aType, offset));
    error = ReadUintTlv<UintType>(aMessage, offset, aValue);

exit:
    return error;
}

// Explicit instantiations of `FindUintTlv<>()`
template otError Tlv::FindUintTlv<uint8_t>(const Message &aMessage, uint8_t aType, uint8_t &aValue);
template otError Tlv::FindUintTlv<uint16_t>(const Message &aMessage, uint8_t aType, uint16_t &aValue);
template otError Tlv::FindUintTlv<uint32_t>(const Message &aMessage, uint8_t aType, uint32_t &aValue);

otError Tlv::FindTlv(const Message &aMessage, uint8_t aType, void *aValue, uint8_t aLength)
{
    otError  error;
    uint16_t offset;
    uint16_t length;

    SuccessOrExit(error = FindTlvValueOffset(aMessage, aType, offset, length));
    VerifyOrExit(length >= aLength, error = OT_ERROR_PARSE);
    aMessage.ReadBytes(offset, aValue, aLength);

exit:
    return error;
}

template <typename UintType> otError Tlv::AppendUintTlv(Message &aMessage, uint8_t aType, UintType aValue)
{
    UintType value = Encoding::BigEndian::HostSwap<UintType>(aValue);

    return AppendTlv(aMessage, aType, &value, sizeof(UintType));
}

// Explicit instantiations of `AppendUintTlv<>()`
template otError Tlv::AppendUintTlv<uint8_t>(Message &aMessage, uint8_t aType, uint8_t aValue);
template otError Tlv::AppendUintTlv<uint16_t>(Message &aMessage, uint8_t aType, uint16_t aValue);
template otError Tlv::AppendUintTlv<uint32_t>(Message &aMessage, uint8_t aType, uint32_t aValue);

otError Tlv::AppendTlv(Message &aMessage, uint8_t aType, const void *aValue, uint8_t aLength)
{
    otError error = OT_ERROR_NONE;
    Tlv     tlv;

    OT_ASSERT(aLength <= Tlv::kBaseTlvMaxLength);

    tlv.SetType(aType);
    tlv.SetLength(aLength);
    SuccessOrExit(error = aMessage.Append(tlv));

    VerifyOrExit(aLength > 0);
    error = aMessage.AppendBytes(aValue, aLength);

exit:
    return error;
}

} // namespace ot
