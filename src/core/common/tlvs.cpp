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

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

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
    uint32_t size = GetSize();

    // OT_ASSERT(size <= UINT16_MAX);

    return aMessage.Append(this, static_cast<uint16_t>(size));
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

    aMessage.Read(offset, aMaxSize, &aTlv);

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

    VerifyOrExit(offset <= remainingLen, OT_NOOP);
    remainingLen -= offset;

    while (true)
    {
        VerifyOrExit(sizeof(Tlv) <= remainingLen, OT_NOOP);
        aMessage.Read(offset, sizeof(Tlv), &tlv);

        if (tlv.mLength != kExtendedLength)
        {
            size = tlv.GetSize();
        }
        else
        {
            ExtendedTlv extTlv;

            VerifyOrExit(sizeof(ExtendedTlv) <= remainingLen, OT_NOOP);
            aMessage.Read(offset, sizeof(ExtendedTlv), &extTlv);

            VerifyOrExit(extTlv.GetLength() <= (remainingLen - sizeof(ExtendedTlv)), OT_NOOP);
            size = extTlv.GetSize();
        }

        VerifyOrExit(size <= remainingLen, OT_NOOP);

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

otError Tlv::ReadUint8Tlv(const Message &aMessage, uint16_t aOffset, uint8_t &aValue)
{
    return ReadTlv(aMessage, aOffset, &aValue, sizeof(uint8_t));
}

otError Tlv::ReadUint16Tlv(const Message &aMessage, uint16_t aOffset, uint16_t &aValue)
{
    otError error;

    SuccessOrExit(error = ReadTlv(aMessage, aOffset, &aValue, sizeof(uint16_t)));
    aValue = HostSwap16(aValue);

exit:
    return error;
}

otError Tlv::ReadUint32Tlv(const Message &aMessage, uint16_t aOffset, uint32_t &aValue)
{
    otError error;

    SuccessOrExit(error = ReadTlv(aMessage, aOffset, &aValue, sizeof(uint32_t)));
    aValue = HostSwap32(aValue);

exit:
    return error;
}

otError Tlv::ReadTlv(const Message &aMessage, uint16_t aOffset, void *aValue, uint8_t aLength)
{
    otError error = OT_ERROR_NONE;
    Tlv     tlv;

    VerifyOrExit(aMessage.Read(aOffset, sizeof(Tlv), &tlv) == sizeof(Tlv), error = OT_ERROR_PARSE);
    VerifyOrExit(!tlv.IsExtended() && (tlv.GetLength() >= aLength), error = OT_ERROR_PARSE);
    VerifyOrExit(tlv.GetSize() + aOffset <= aMessage.GetLength(), error = OT_ERROR_PARSE);

    aMessage.Read(aOffset + sizeof(Tlv), aLength, aValue);

exit:
    return error;
}

otError Tlv::FindUint8Tlv(const Message &aMessage, uint8_t aType, uint8_t &aValue)
{
    otError  error = OT_ERROR_NONE;
    uint16_t offset;

    SuccessOrExit(error = FindTlvOffset(aMessage, aType, offset));
    error = ReadUint8Tlv(aMessage, offset, aValue);

exit:
    return error;
}

otError Tlv::FindUint16Tlv(const Message &aMessage, uint8_t aType, uint16_t &aValue)
{
    otError  error = OT_ERROR_NONE;
    uint16_t offset;

    SuccessOrExit(error = FindTlvOffset(aMessage, aType, offset));
    error = ReadUint16Tlv(aMessage, offset, aValue);

exit:
    return error;
}

otError Tlv::FindUint32Tlv(const Message &aMessage, uint8_t aType, uint32_t &aValue)
{
    otError  error = OT_ERROR_NONE;
    uint16_t offset;

    SuccessOrExit(error = FindTlvOffset(aMessage, aType, offset));
    error = ReadUint32Tlv(aMessage, offset, aValue);

exit:
    return error;
}

otError Tlv::FindTlv(const Message &aMessage, uint8_t aType, void *aValue, uint8_t aLength)
{
    otError  error;
    uint16_t offset;
    uint16_t length;

    SuccessOrExit(error = FindTlvValueOffset(aMessage, aType, offset, length));
    VerifyOrExit(length >= aLength, error = OT_ERROR_PARSE);
    aMessage.Read(offset, aLength, static_cast<uint8_t *>(aValue));

exit:
    return error;
}

otError Tlv::AppendUint8Tlv(Message &aMessage, uint8_t aType, uint8_t aValue)
{
    uint8_t value8 = aValue;

    return AppendTlv(aMessage, aType, &value8, sizeof(uint8_t));
}

otError Tlv::AppendUint16Tlv(Message &aMessage, uint8_t aType, uint16_t aValue)
{
    uint16_t value16 = HostSwap16(aValue);

    return AppendTlv(aMessage, aType, &value16, sizeof(uint16_t));
}

otError Tlv::AppendUint32Tlv(Message &aMessage, uint8_t aType, uint32_t aValue)
{
    uint32_t value32 = HostSwap32(aValue);

    return AppendTlv(aMessage, aType, &value32, sizeof(uint32_t));
}

otError Tlv::AppendTlv(Message &aMessage, uint8_t aType, const void *aValue, uint8_t aLength)
{
    otError error = OT_ERROR_NONE;
    Tlv     tlv;

    OT_ASSERT(aLength <= Tlv::kBaseTlvMaxLength);

    tlv.SetType(aType);
    tlv.SetLength(aLength);
    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));

    VerifyOrExit(aLength > 0, OT_NOOP);
    error = aMessage.Append(aValue, aLength);

exit:
    return error;
}

} // namespace ot
