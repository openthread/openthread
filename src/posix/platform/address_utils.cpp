/*
 *  Copyright (c) 2020, The OpenThread Authors.
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

#include "address_utils.hpp"

#include "net/ip6_address.hpp"

namespace ot {
namespace Posix {

static const uint8_t kAllOnes[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct in6_addr PrefixLengthToNetmask(uint8_t aPrefixLength)
{
    static constexpr uint8_t kMaxPrefixLength = (OT_IP6_ADDRESS_SIZE * CHAR_BIT);
    struct in6_addr          ret;
    ot::Ip6::Address         addr;

    if (aPrefixLength > kMaxPrefixLength)
    {
        aPrefixLength = kMaxPrefixLength;
    }

    addr.Clear();
    addr.SetPrefix(kAllOnes, aPrefixLength);
    memcpy(&ret, addr.mFields.m8, sizeof(addr.mFields.m8));

    return ret;
}

uint8_t NetmaskToPrefixLength(const struct sockaddr_in6 *netmask)
{
    return otIp6PrefixMatch(reinterpret_cast<const otIp6Address *>(netmask->sin6_addr.s6_addr),
                            reinterpret_cast<const otIp6Address *>(kAllOnes));
}

} // namespace Posix
} // namespace ot
