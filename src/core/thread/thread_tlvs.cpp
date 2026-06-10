/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file implements methods for generating and processing Thread Network Layer TLVs.
 */

#include "thread_tlvs.hpp"

namespace ot {

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2

Error Ip6AddressesTlv::AppendTo(Message &aMessage, const Ip6::Address *aAddresses, uint16_t aNumAddresses)
{
    Error         error;
    Tlv::Bookmark tlvBookmark;

    SuccessOrExit(error = Tlv::StartTlv(aMessage, kType, tlvBookmark));
    SuccessOrExit(error = aMessage.AppendBytes(aAddresses, aNumAddresses * sizeof(Ip6::Address)));
    error = Tlv::EndTlv(aMessage, tlvBookmark);

exit:
    return error;
}

Error Ip6AddressesTlv::FindIn(const Message &aMessage, Mlr::AddressArray &aAddresses)
{
    Error       error;
    OffsetRange offsetRange;

    aAddresses.Clear();

    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(aMessage, kType, offsetRange));

    while (!offsetRange.IsEmpty())
    {
        Ip6::Address address;

        SuccessOrExit(error = aMessage.ReadAndAdvance(offsetRange, address));
        SuccessOrExit(error = aAddresses.AddUnique(address));
    }

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2

} // namespace ot
