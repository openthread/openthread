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
    uint32_t size = GetSize();

    OT_ASSERT(size <= UINT16_MAX);

    return aMessage.Append(this, static_cast<uint16_t>(size));
}

otError Tlv::Get(const Message &aMessage, uint8_t aType, uint16_t aMaxSize, Tlv &aTlv)
{
    otError  error;
    uint16_t offset;
    uint16_t size;

    SuccessOrExit(error = Find(aMessage, aType, &offset, &size, NULL));

    if (aMaxSize > size)
    {
        aMaxSize = size;
    }

    aMessage.Read(offset, aMaxSize, &aTlv);

exit:
    return error;
}

otError Tlv::GetOffset(const Message &aMessage, uint8_t aType, uint16_t &aOffset)
{
    return Find(aMessage, aType, &aOffset, NULL, NULL);
}

otError Tlv::GetValueOffset(const Message &aMessage, uint8_t aType, uint16_t &aValueOffset, uint16_t &aLength)
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
            if (aOffset != NULL)
            {
                *aOffset = offset;
            }

            if (aSize != NULL)
            {
                *aSize = static_cast<uint16_t>(size);
            }

            if (aIsExtendedTlv != NULL)
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

otError Tlv::ReadUint8Tlv(const Message &aMessage, uint8_t aType, uint8_t &aValue)
{
    otError  error = OT_ERROR_NONE;
    TlvUint8 tlv8;

    SuccessOrExit(error = Get(aMessage, aType, sizeof(tlv8), tlv8));
    VerifyOrExit(tlv8.IsValid(), error = OT_ERROR_PARSE);
    aValue = tlv8.GetUint8Value();

exit:
    return error;
}

otError Tlv::ReadUint16Tlv(const Message &aMessage, uint8_t aType, uint16_t &aValue)
{
    otError   error = OT_ERROR_NONE;
    TlvUint16 tlv16;

    SuccessOrExit(error = Get(aMessage, aType, sizeof(tlv16), tlv16));
    VerifyOrExit(tlv16.IsValid(), error = OT_ERROR_PARSE);
    aValue = tlv16.GetUint16Value();

exit:
    return error;
}

otError Tlv::ReadUint32Tlv(const Message &aMessage, uint8_t aType, uint32_t &aValue)
{
    otError   error = OT_ERROR_NONE;
    TlvUint32 tlv32;

    SuccessOrExit(error = Get(aMessage, aType, sizeof(tlv32), tlv32));
    VerifyOrExit(tlv32.IsValid(), error = OT_ERROR_PARSE);
    aValue = tlv32.GetUint32Value();

exit:
    return error;
}

otError Tlv::ReadTlv(const Message &aMessage, uint8_t aType, void *aValue, uint8_t aLength)
{
    otError  error;
    uint16_t offset;
    uint16_t length;

    SuccessOrExit(error = GetValueOffset(aMessage, aType, offset, length));
    VerifyOrExit(length >= aLength, error = OT_ERROR_PARSE);
    aMessage.Read(offset, aLength, static_cast<uint8_t *>(aValue));

exit:
    return error;
}

otError Tlv::AppendUint8Tlv(Message &aMessage, uint8_t aType, uint8_t aValue)
{
    TlvUint8 tlv8;

    tlv8.Init(aType);
    tlv8.SetUint8Value(aValue);

    return tlv8.AppendTo(aMessage);
}

otError Tlv::AppendUint16Tlv(Message &aMessage, uint8_t aType, uint16_t aValue)
{
    TlvUint16 tlv16;

    tlv16.Init(aType);
    tlv16.SetUint16Value(aValue);

    return tlv16.AppendTo(aMessage);
}

otError Tlv::AppendUint32Tlv(Message &aMessage, uint8_t aType, uint32_t aValue)
{
    TlvUint32 tlv32;

    tlv32.Init(aType);
    tlv32.SetUint32Value(aValue);

    return tlv32.AppendTo(aMessage);
}

otError Tlv::AppendTlv(Message &aMessage, uint8_t aType, const uint8_t *aValue, uint8_t aLength)
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
