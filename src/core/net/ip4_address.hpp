/*
 *  Copyright (c) 2021-2022, The OpenThread Authors.
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
 *   This file includes definitions for IPv4 addresses.
 */

#ifndef IP4_ADDRESS_HPP_
#define IP4_ADDRESS_HPP_

#include "openthread-core-config.h"

#include <openthread/nat64.h>

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/equatable.hpp"
#include "common/error.hpp"
#include "common/string.hpp"

namespace ot {

namespace Ip6 {
// Forward declation for SynthesizeFromIp4Address
class Address;
} // namespace Ip6

namespace Ip4 {

using Encoding::BigEndian::HostSwap16;
using Encoding::BigEndian::HostSwap32;

/**
 * This class represents an IPv4 address.
 *
 */
OT_TOOL_PACKED_BEGIN
class Address : public otIp4Address, public Equatable<Address>, public Clearable<Address>
{
public:
    static constexpr uint16_t kSize              = 4;  ///< Size of an IPv4 Address (in bytes).
    static constexpr uint16_t kAddressStringSize = 17; ///< String size used by `ToString()`.

    /**
     * This type defines the fixed-length `String` object returned from `ToString()`.
     *
     */
    typedef String<kAddressStringSize> InfoString;

    /**
     * This method gets the IPv4 address as a pointer to a byte array.
     *
     * @returns A pointer to a byte array containing the IPv4 address.
     *
     */
    const uint8_t *GetBytes(void) const { return mFields.m8; }

    /**
     * This method sets the IPv4 address from a given byte array.
     *
     * @param[in] aBuffer    Pointer to an array containing the IPv4 address. `kSize` bytes from the buffer
     *                       are copied to form the IPv4 address.
     *
     */
    void SetBytes(const uint8_t *aBuffer) { memcpy(mFields.m8, aBuffer, kSize); }

    /**
     * This method sets the host field of an IPv4 address.
     *
     * @param[in] aBuffer    Pointer to an array containing the IPv4 address. `kSize` bytes from the buffer
     *                       are copied to form the IPv4 address.
     *
     */
    Error SetHost(const uint32_t aPrefixLength, const uint32_t aHost)
    {
        if (aHost >= static_cast<uint32_t>((1 << aPrefixLength) - 1) || aHost == 0)
        {
            return kErrorParse;
        }
        uint32_t hostMask = ((1 << aPrefixLength) - 1);
        mFields.m32       = HostSwap32(((HostSwap32(mFields.m32) & ~hostMask) | aHost));
        return kErrorNone;
    }

    /**
     * This method sets the IPv4 address by performing NAT64 address translation from a given IPv6 address as specified
     * in RFC 6052.
     *
     * The NAT64 @p aPrefixLength MUST be one of the following values: 32, 40, 48, 56, 64, or 96, otherwise the behavior
     * of this method is undefined.
     *
     * @param[in] aPrefixLength      The prefix length to use for IPv4/IPv6 translation.
     * @param[in] aIp6Address  The IPv6 address to translate to IPv4.
     *
     */
    void SynthesizeFromIp6Address(uint8_t aPrefixLength, Ip6::Address &aIp6Address);

    /**
     * This method parses an IPv4 address string.
     *
     * The string MUST follow the quad-dotted notation of four decimal values (ranging from 0 to 255 each). For
     * example, "127.0.0.1"
     *
     * @param[in]  aString        A pointer to the null-terminated string.
     *
     * @retval kErrorNone         Successfully parsed the IPv4 address string.
     * @retval kErrorParse        Failed to parse the IPv4 address string.
     *
     */
    Error FromString(const char *aString);

    /**
     * This method converts the IPv4 address to a string.
     *
     * The string format uses quad-dotted notation of four bytes in the address (e.g., "127.0.0.1").
     *
     * @returns An `InfoString` representing the IPv4 address.
     *
     */
    InfoString ToString(void) const;
} OT_TOOL_PACKED_END;

class Cidr : public otIp4Cidr, public Unequatable<Cidr>, public Clearable<Address>
{
public:
    static constexpr uint16_t kCidrSuffixSize = 3; ///< Suffix to represent CIDR (/dd).

    /**
     * This type defines the fixed-length `String` object returned from `ToString()`.
     *
     */
    typedef String<Address::kAddressStringSize + kCidrSuffixSize> InfoString;

    /**
     * This method converts the IPv4 address to a string.
     *
     * The string format uses quad-dotted notation of four bytes in the address (e.g., "127.0.0.1").
     *
     * @returns An `InfoString` representing the IPv4 address.
     *
     */
    InfoString ToString(void) const;

    /**
     * This method returns the host mask (bitwise not of the subnet mask) of the CIDR.
     *
     * @returns A uint32 for the host mask, in network byte order.
     */
    inline uint32_t HostMask() const { return HostSwap32((uint32_t(1) << uint32_t(32 - mLength)) - 1); }

    /**
     * This method returns the subnet mask of the CIDR.
     *
     * @returns A uint32 for the subnet mask, in network byte order.
     */
    inline uint32_t SubnetMask() const { return ~HostMask(); }

    /**
     * This method gets the prefix as a pointer to a byte array.
     *
     * @returns A pointer to a byte array containing the Prefix.
     *
     */
    const uint8_t *GetBytes(void) const { return mAddress.mFields.m8; }

    /**
     * This method overloads operator `==` to evaluate whether or not two prefixes are equal.
     *
     * @param[in]  aOther  The other prefix to compare with.
     *
     * @retval TRUE   If the two prefixes are equal.
     * @retval FALSE  If the two prefixes are not equal.
     *
     */
    bool operator==(const Cidr &aOther) const;

    /**
     * This method sets the CIDR.
     *
     * @param[in] aAddress  A pointer to buffer containing the CIDR bytes. The length of aAddress should be 4 bytes.
     * @param[in] aLength  The length of CIDR in bits.
     *
     */
    void Set(const uint8_t *aAddress, uint8_t aLength);

    /**
     * This method returns an IPv4 address within the CIDR block.
     *
     * @param[in] aHost The host bits of the IPv4 address in host byte order. The aHost will be masked by host mask.
     */
    Address Host(uint32_t aHost) const;
};
} // namespace Ip4

DefineCoreType(otIp4Address, Ip4::Address);
DefineCoreType(otIp4Cidr, Ip4::Cidr);

} // namespace ot

#endif // IP4_ADDRESS_HPP_
