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
 *   This file implements common methods for manipulating MLE TLVs.
 */

#include <common/code_utils.hpp>
#include <common/message.hpp>
#include <thread/mle_tlvs.hpp>

namespace Thread {
namespace Mle {

ThreadError Tlv::GetTlv(const Message &aMessage, Type aType, uint16_t aMaxLength, Tlv &aTlv)
{
    ThreadError error = kThreadError_Parse;
    uint16_t offset;

    SuccessOrExit(error = GetOffset(aMessage, aType, offset));
    aMessage.Read(offset, sizeof(Tlv), &aTlv);

    if (aMaxLength > sizeof(aTlv) + aTlv.GetLength())
    {
        aMaxLength = sizeof(aTlv) + (uint16_t)aTlv.GetLength();
    }

    aMessage.Read(offset, aMaxLength, &aTlv);

exit:
    return error;
}

ThreadError Tlv::GetOffset(const Message &aMessage, Type aType, uint16_t &aOffset)
{
    ThreadError error = kThreadError_Parse;
    uint16_t offset = aMessage.GetOffset();
    uint16_t end = aMessage.GetLength();
    Tlv tlv;

    while (offset < end)
    {
        aMessage.Read(offset, sizeof(Tlv), &tlv);

        if (tlv.GetType() == aType && (offset + sizeof(tlv) + tlv.GetLength()) <= end)
        {
            aOffset = offset;
            ExitNow(error = kThreadError_None);
        }

        offset += (uint16_t)(sizeof(tlv) + tlv.GetLength());
    }

exit:
    return error;
}

}  // namespace Mle
}  // namespace Thread
