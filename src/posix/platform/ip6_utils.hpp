/*
 *  Copyright (c) 2021, The OpenThread Authors.
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

#ifndef OT_POSIX_PLATFORM_IP6_UTILS_HPP_
#define OT_POSIX_PLATFORM_IP6_UTILS_HPP_

#include "openthread-posix-config.h"
#include "platform-posix.h"

#include <arpa/inet.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <openthread/ip6.h>

#include "common/code_utils.hpp"

namespace ot {
namespace Posix {
namespace Ip6Utils {

/**
 * Indicates whether or not the IPv6 address scope is Link-Local.
 *
 * @param[in] aAddress   The IPv6 address to check.
 *
 * @retval TRUE   If the IPv6 address scope is Link-Local.
 * @retval FALSE  If the IPv6 address scope is not Link-Local.
 */
inline bool IsIp6AddressLinkLocal(const otIp6Address &aAddress)
{
    return (aAddress.mFields.m8[0] == 0xfe) && ((aAddress.mFields.m8[1] & 0xc0) == 0x80);
}

/**
 * Indicates whether or not the IPv6 address is multicast.
 *
 * @param[in] aAddress   The IPv6 address to check.
 *
 * @retval TRUE   If the IPv6 address scope is multicast.
 * @retval FALSE  If the IPv6 address scope is not multicast.
 */
inline bool IsIp6AddressMulticast(const otIp6Address &aAddress) { return (aAddress.mFields.m8[0] == 0xff); }

/**
 * Indicates whether or not the IPv6 address is unspecified.
 *
 * @param[in] aAddress   The IPv6 address to check.
 *
 * @retval TRUE   If the IPv6 address scope is unspecified.
 * @retval FALSE  If the IPv6 address scope is not unspecified.
 */
inline bool IsIp6AddressUnspecified(const otIp6Address &aAddress) { return otIp6IsAddressUnspecified(&aAddress); }

/**
 * Copies the IPv6 address bytes into a given buffer.
 *
 * @param[in] aAddress  The IPv6 address to copy.
 * @param[in] aBuffer   A pointer to buffer to copy the address to.
 */
inline void CopyIp6AddressTo(const otIp6Address &aAddress, void *aBuffer)
{
    memcpy(aBuffer, &aAddress, sizeof(otIp6Address));
}

/**
 * Reads and set the IPv6 address bytes from a given buffer.
 *
 * @param[in] aBuffer    A pointer to buffer to read from.
 * @param[out] aAddress  A reference to populate with the read IPv6 address.
 */
inline void ReadIp6AddressFrom(const void *aBuffer, otIp6Address &aAddress)
{
    memcpy(&aAddress, aBuffer, sizeof(otIp6Address));
}

constexpr size_t kIp6PayloadLengthOffset = 4;  ///< Offset of Payload Length in IPv6 header.
constexpr size_t kIp6HopLimitOffset      = 7;  ///< Offset of Hop Limit in IPv6 header.
constexpr size_t kIp6SourceOffset        = 8;  ///< Offset of Source Address in IPv6 header.
constexpr size_t kIp6DestinationOffset   = 24; ///< Offset of Destination Address in IPv6 header.

/**
 * Sums a sequence of bytes in network byte order into a 32-bit one's complement accumulator.
 *
 * @param[in] aBytes   A pointer to the byte sequence.
 * @param[in] aLength  The number of bytes to sum.
 *
 * @returns The 32-bit sum prior to 16-bit carry folding.
 */
inline uint32_t SumChecksumBytes(const uint8_t *aBytes, size_t aLength)
{
    uint32_t sum = 0;

    for (size_t i = 0; i + 1 < aLength; i += 2)
    {
        sum += (static_cast<uint32_t>(aBytes[i]) << 8) | aBytes[i + 1];
    }

    if ((aLength & 1) != 0)
    {
        sum += static_cast<uint32_t>(aBytes[aLength - 1]) << 8;
    }

    return sum;
}

/**
 * Recomputes the UDP or TCP checksum in an unfragmented IPv6 packet without extension headers.
 *
 * Packets with other next-header values or truncated lengths are left unmodified.
 *
 * @param[in,out] aPacket  A pointer to the IPv6 packet buffer.
 * @param[in]     aLength  The length of the packet buffer in bytes.
 */
inline void UpdateIp6TransportChecksum(uint8_t *aPacket, size_t aLength)
{
    constexpr uint16_t kUdpHeaderSize     = 8;
    constexpr size_t   kUdpLengthOffset   = 4;
    constexpr size_t   kUdpChecksumOffset = 6;
    constexpr uint16_t kTcpHeaderSize     = 20;
    constexpr size_t   kTcpChecksumOffset = 16;

    uint16_t payloadLength;
    uint16_t upperLayerLength = 0;
    uint8_t  nextHeader;
    size_t   checksumOffset = 0;
    uint32_t sum            = 0;
    uint16_t checksum;

    VerifyOrExit(aPacket != nullptr);
    VerifyOrExit(aLength >= OT_IP6_HEADER_SIZE);
    VerifyOrExit((aPacket[0] >> 4) == 6);

    payloadLength =
        (static_cast<uint16_t>(aPacket[kIp6PayloadLengthOffset]) << 8) | aPacket[kIp6PayloadLengthOffset + 1];
    VerifyOrExit(OT_IP6_HEADER_SIZE + static_cast<size_t>(payloadLength) <= aLength);

    nextHeader = aPacket[OT_IP6_HEADER_PROTO_OFFSET];

    switch (nextHeader)
    {
    case OT_IP6_PROTO_UDP:
        VerifyOrExit(payloadLength >= kUdpHeaderSize);
        upperLayerLength = (static_cast<uint16_t>(aPacket[OT_IP6_HEADER_SIZE + kUdpLengthOffset]) << 8) |
                           aPacket[OT_IP6_HEADER_SIZE + kUdpLengthOffset + 1];
        VerifyOrExit((upperLayerLength >= kUdpHeaderSize) && (upperLayerLength <= payloadLength));
        checksumOffset = OT_IP6_HEADER_SIZE + kUdpChecksumOffset;
        break;

    case OT_IP6_PROTO_TCP:
        VerifyOrExit(payloadLength >= kTcpHeaderSize);
        upperLayerLength = payloadLength;
        checksumOffset   = OT_IP6_HEADER_SIZE + kTcpChecksumOffset;
        break;

    default:
        ExitNow();
    }

    aPacket[checksumOffset]     = 0;
    aPacket[checksumOffset + 1] = 0;

    sum += SumChecksumBytes(&aPacket[kIp6SourceOffset], 2 * OT_IP6_ADDRESS_SIZE);
    sum += upperLayerLength;
    sum += nextHeader;
    sum += SumChecksumBytes(&aPacket[OT_IP6_HEADER_SIZE], upperLayerLength);

    while ((sum >> 16) != 0)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    checksum = static_cast<uint16_t>(~sum);

    if ((nextHeader == OT_IP6_PROTO_UDP) && (checksum == 0))
    {
        checksum = 0xffff;
    }

    aPacket[checksumOffset]     = static_cast<uint8_t>(checksum >> 8);
    aPacket[checksumOffset + 1] = static_cast<uint8_t>(checksum & 0xff);

exit:
    return;
}

/**
 * This utility class converts binary IPv6 address to text format.
 */
class Ip6AddressString
{
public:
    /**
     * The constructor of this converter.
     *
     * @param[in]   aAddress    A pointer to a buffer holding an IPv6 address.
     */
    Ip6AddressString(const void *aAddress)
    {
        VerifyOrDie(inet_ntop(AF_INET6, aAddress, mBuffer, sizeof(mBuffer)) != nullptr, OT_EXIT_ERROR_ERRNO);
    }

    /**
     * Returns the string as a null-terminated C string.
     *
     * @returns The null-terminated C string.
     */
    const char *AsCString(void) const { return mBuffer; }

private:
    char mBuffer[INET6_ADDRSTRLEN];
};

} // namespace Ip6Utils
} // namespace Posix
} // namespace ot

#endif // OT_POSIX_PLATFORM_IP6_UTILS_HPP_
