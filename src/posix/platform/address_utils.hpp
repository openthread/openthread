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

#ifndef POSIX_PLATFORM_ADDRESS_UTILS_HPP_
#define POSIX_PLATFORM_ADDRESS_UTILS_HPP_

#include "openthread-posix-config.h"

#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <openthread/ip6.h>

#include "lib/platform/exit_code.h"

namespace ot {

namespace Posix {

/**
 * This utility class converts binary IPv6 address to text format.
 *
 */
class Ip6AddressString
{
public:
    /**
     * The constructor of this converter.
     *
     * @param[in]   aAddress    A pointer to a buffer holding an IPv6 address.
     *
     */
    explicit Ip6AddressString(const void *aAddress)
    {
        VerifyOrDie(inet_ntop(AF_INET6, aAddress, mBuffer, sizeof(mBuffer)) != nullptr, OT_EXIT_ERROR_ERRNO);
    }

    explicit Ip6AddressString(const otIp6Address &aAddress)
        : Ip6AddressString(&aAddress)
    {
    }

    /**
     * The constructor of this converter.
     *
     * @param[in]   aAddress    A pointer to a buffer holding an IPv6 address.
     *
     */
    explicit Ip6AddressString(const in6_addr &aAddress)
        : Ip6AddressString(reinterpret_cast<const void *>(&aAddress))
    {
    }

    /**
     * This method returns the string as a null-terminated C string.
     *
     * @returns The null-terminated C string.
     *
     */
    const char *AsCString(void) const { return mBuffer; }

private:
    char mBuffer[INET6_ADDRSTRLEN];
};

/**
 * This utility class converts binary IPv6 prefix to text format.
 *
 */
class Ip6PrefixString
{
public:
    /**
     * The constructor of this converter.
     *
     * @param[in]   aPrefix       A pointer to a buffer holding an IPv6 prefix.
     * @param[in]  aPrefixLength  The length of the prefix (in bits).
     *
     */
    Ip6PrefixString(const uint8_t *aPrefix, uint8_t aPrefixLength)
    {
        VerifyOrDie(inet_ntop(AF_INET6, aPrefix, mBuffer, sizeof(mBuffer)) != nullptr, OT_EXIT_ERROR_ERRNO);
        sprintf(mBuffer + strlen(mBuffer), "/%hhu", aPrefixLength);
    }

    explicit Ip6PrefixString(const otIp6Prefix &aPrefix)
        : Ip6PrefixString(aPrefix.mPrefix.mFields.m8, aPrefix.mLength)
    {
    }

    explicit Ip6PrefixString(const otIp6AddressInfo &aAddressInfo)
        : Ip6PrefixString(aAddressInfo.mAddress->mFields.m8, aAddressInfo.mPrefixLength)
    {
    }

    /**
     * This method returns the IPv6 prefix as a null-terminated C string.
     *
     * @returns The null-terminated C string.
     *
     */
    const char *AsCString(void) const { return mBuffer; }

private:
    char mBuffer[INET6_ADDRSTRLEN + 4];
};

struct in6_addr PrefixLengthToNetmask(uint8_t aPrefixLength);
uint8_t         NetmaskToPrefixLength(const struct sockaddr_in6 *aNetmask);

} // namespace Posix

} // namespace ot

#endif // POSIX_PLATFORM_ADDRESS_UTILS_HPP_
