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
#include "common/message.hpp"

namespace ot {

otError Tlv::Get(const Message &aMessage, uint8_t aType, uint16_t aMaxLength, Tlv &aTlv)
{
    otError  error = OT_ERROR_NOT_FOUND;
    uint16_t offset;

    SuccessOrExit(error = GetOffset(aMessage, aType, offset));
    aMessage.Read(offset, sizeof(Tlv), &aTlv);

    if (aMaxLength > sizeof(aTlv) + aTlv.GetLength())
    {
        aMaxLength = sizeof(aTlv) + aTlv.GetLength();
    }

    aMessage.Read(offset, aMaxLength, &aTlv);

exit:
    return error;
}

otError Tlv::GetOffset(const Message &aMessage, uint8_t aType, uint16_t &aOffset)
{
    otError  error  = OT_ERROR_NOT_FOUND;
    uint16_t offset = aMessage.GetOffset();
    uint16_t end    = aMessage.GetLength();
    Tlv      tlv;

    while (offset + sizeof(tlv) <= end)
    {
        uint32_t length = sizeof(tlv);

        aMessage.Read(offset, sizeof(tlv), &tlv);

        if (tlv.GetLength() != kExtendedLength)
        {
            length += tlv.GetLength();
        }
        else
        {
            uint16_t extLength;

            VerifyOrExit(sizeof(extLength) == aMessage.Read(offset + sizeof(tlv), sizeof(extLength), &extLength));
            length += sizeof(extLength) + HostSwap16(extLength);
        }

        VerifyOrExit(offset + length <= end);

        if (tlv.GetType() == aType)
        {
            aOffset = offset;
            ExitNow(error = OT_ERROR_NONE);
        }

        offset += static_cast<uint16_t>(length);
    }

exit:
    return error;
}

otError Tlv::GetValueOffset(const Message &aMessage, uint8_t aType, uint16_t &aOffset, uint16_t &aLength)
{
    otError  error  = OT_ERROR_NOT_FOUND;
    uint16_t offset = aMessage.GetOffset();
    uint16_t end    = aMessage.GetLength();
    Tlv      tlv;

    while (offset + sizeof(tlv) <= end)
    {
        uint16_t length;

        aMessage.Read(offset, sizeof(tlv), &tlv);
        offset += sizeof(tlv);
        length = tlv.GetLength();

        if (length == kExtendedLength)
        {
            VerifyOrExit(offset + sizeof(length) <= end);
            aMessage.Read(offset, sizeof(length), &length);
            offset += sizeof(length);
            length = HostSwap16(length);
        }

        VerifyOrExit(length <= end - offset);

        if (tlv.GetType() == aType)
        {
            aOffset = offset;
            aLength = length;
            ExitNow(error = OT_ERROR_NONE);
        }

        offset += length;
    }

exit:
    return error;
}

} // namespace ot
