/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file includes definitions for IPv4 packet processing.
 */

#ifndef IP4_TYPES_HPP_
#define IP4_TYPES_HPP_

#include "openthread-core-config.h"

#include <stddef.h>

#include <openthread/nat64.h>

#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/message.hpp"
#include "net/ip6_types.hpp"
#include "net/netif.hpp"
#include "net/socket.hpp"
#include "net/tcp6.hpp"
#include "net/udp6.hpp"

namespace ot {

namespace Ip6 {
// Forward declaration for ExtractFromIp4Address
class Address;
} // namespace Ip6

/**
 * @namespace ot::Ip4
 *
 * @brief
 *   This namespace includes definitions for IPv4 networking used by NAT64.
 *
 */
namespace Ip4 {

using Ecn = Ip6::Ecn;

/**
 * @addtogroup core-ipv4
 *
 * @brief
 *   This module includes definitions for the IPv4 network layer.
 *
 */

/**
 * @addtogroup core-ip4-ip4
 *
 * @brief
 *   This module includes definitions for IPv4 networking used by NAT64.
 *
 * @{
 *
 */

// Forward declaration for Address::SynthesizeFromCidrAndHost
class Cidr;

/**
 * Represents an IPv4 address.
 *
 */
OT_TOOL_PACKED_BEGIN
class Address : public otIp4Address, public Equatable<Address>, public Clearable<Address>
{
public:
    static constexpr uint16_t kSize              = 4;  ///< Size of an IPv4 Address (in bytes).
    static constexpr uint16_t kAddressStringSize = 17; ///< String size used by `ToString()`.

    /**
     * Defines the fixed-length `String` object returned from `ToString()`.
     *
     */
    typedef String<kAddressStringSize> InfoString;

    /**
     * Gets the IPv4 address as a pointer to a byte array.
     *
     * @returns A pointer to a byte array containing the IPv4 address.
     *
     */
    const uint8_t *GetBytes(void) const { return mFields.m8; }

    /**
     * Sets the IPv4 address from a given byte array.
     *
     * @param[in] aBuffer    Pointer to an array containing the IPv4 address. `kSize` bytes from the buffer
     *                       are copied to form the IPv4 address.
     *
     */
    void SetBytes(const uint8_t *aBuffer) { memcpy(mFields.m8, aBuffer, kSize); }

    /**
     * Sets the IPv4 address from a given IPv4-mapped IPv6 address.
     *
     * @param[in] aIp6Address  An IPv6 address.
     *
     * @retval kErrorNone  Set the IPv4 address successfully.
     * @retval kErrorPase  The @p aIp6Address does not follow the IPv4-mapped IPv6 address format.
     *
     */
    Error ExtractFromIp4MappedIp6Address(const Ip6::Address &aIp6Address);

    /**
     * Sets the IPv4 address by performing NAT64 address translation from a given IPv6 address as specified
     * in RFC 6052.
     *
     * The NAT64 @p aPrefixLength MUST be one of the following values: 32, 40, 48, 56, 64, or 96, otherwise the behavior
     * of this method is undefined.
     *
     * @param[in] aPrefixLength      The prefix length to use for IPv4/IPv6 translation.
     * @param[in] aIp6Address  The IPv6 address to translate to IPv4.
     *
     */
    void ExtractFromIp6Address(uint8_t aPrefixLength, const Ip6::Address &aIp6Address);

    /**
     * Sets the IPv4 address from the given CIDR and the host field.
     *
     * @param[in] aCidr The CIDR for the IPv4 address.
     * @param[in] aHost The host bits of the IPv4 address in host byte order. The aHost will be masked by host mask.
     *
     */
    void SynthesizeFromCidrAndHost(const Cidr &aCidr, uint32_t aHost);

    /**
     * Parses an IPv4 address string terminated by `aTerminatorChar`.
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
    Error FromString(const char *aString, char aTerminatorChar = kNullChar);

    /**
     * Converts the address to a string.
     *
     * The string format uses quad-dotted notation of four bytes in the address (e.g., "127.0.0.1").
     *
     * If the resulting string does not fit in @p aBuffer (within its @p aSize characters), the string will be
     * truncated but the outputted string is always null-terminated.
     *
     * @param[out] aBuffer   A pointer to a char array to output the string (MUST NOT be `nullptr`).
     * @param[in]  aSize     The size of @p aBuffer (in bytes).
     *
     */
    void ToString(char *aBuffer, uint16_t aSize) const;

    /**
     * Converts the IPv4 address to a string.
     *
     * The string format uses quad-dotted notation of four bytes in the address (e.g., "127.0.0.1").
     *
     * @returns An `InfoString` representing the IPv4 address.
     *
     */
    InfoString ToString(void) const;

private:
    void ToString(StringWriter &aWriter) const;
} OT_TOOL_PACKED_END;

/**
 * Represents an IPv4 CIDR block.
 *
 */
class Cidr : public otIp4Cidr, public Unequatable<Cidr>, public Clearable<Address>
{
    friend class Address;

public:
    static constexpr uint16_t kCidrSuffixSize = 3; ///< Suffix to represent CIDR (/dd).

    /**
     * Defines the fixed-length `String` object returned from `ToString()`.
     *
     */
    typedef String<Address::kAddressStringSize + kCidrSuffixSize> InfoString;

    /**
     * Converts the IPv4 CIDR string to binary.
     *
     * The string format uses quad-dotted notation of four bytes in the address with the length of prefix (e.g.,
     * "127.0.0.1/32").
     *
     * @param[in]  aString  A pointer to the null-terminated string.
     *
     * @retval kErrorNone          Successfully parsed the IPv4 CIDR string.
     * @retval kErrorParse         Failed to parse the IPv4 CIDR string.
     *
     */
    Error FromString(const char *aString);

    /**
     * Converts the IPv4 CIDR to a string.
     *
     * The string format uses quad-dotted notation of four bytes in the address with the length of prefix (e.g.,
     * "127.0.0.1/32").
     *
     * If the resulting string does not fit in @p aBuffer (within its @p aSize characters), the string will be
     * truncated but the outputted string is always null-terminated.
     *
     * @param[out] aBuffer   A pointer to a char array to output the string (MUST NOT be `nullptr`).
     * @param[in]  aSize     The size of @p aBuffer (in bytes).
     *
     */
    void ToString(char *aBuffer, uint16_t aSize) const;

    /**
     * Converts the IPv4 CIDR to a string.
     *
     * The string format uses quad-dotted notation of four bytes in the address with the length of prefix (e.g.,
     * "127.0.0.1/32").
     *
     * @returns An `InfoString` representing the IPv4 cidr.
     *
     */
    InfoString ToString(void) const;

    /**
     * Gets the prefix as a pointer to a byte array.
     *
     * @returns A pointer to a byte array containing the Prefix.
     *
     */
    const uint8_t *GetBytes(void) const { return mAddress.mFields.m8; }

    /**
     * Overloads operator `==` to evaluate whether or not two prefixes are equal.
     *
     * @param[in]  aOther  The other prefix to compare with.
     *
     * @retval TRUE   If the two prefixes are equal.
     * @retval FALSE  If the two prefixes are not equal.
     *
     */
    bool operator==(const Cidr &aOther) const;

    /**
     * Sets the CIDR.
     *
     * @param[in] aAddress  A pointer to buffer containing the CIDR bytes. The length of aAddress should be 4 bytes.
     * @param[in] aLength   The length of CIDR in bits.
     *
     */
    void Set(const uint8_t *aAddress, uint8_t aLength);

private:
    uint32_t HostMask(void) const
    {
        // Note: Using LL suffix to make it a uint64 since /32 is a valid CIDR, and right shifting 32 bits is undefined
        // for uint32.
        return BigEndian::HostSwap32(0xffffffffLL >> mLength);
    }

    uint32_t SubnetMask(void) const { return ~HostMask(); }

    void ToString(StringWriter &aWriter) const;
};

/**
 * Implements IPv4 header generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class Header : public Clearable<Header>
{
public:
    static constexpr uint8_t kVersionIhlOffset         = 0;
    static constexpr uint8_t kTrafficClassOffset       = 1;
    static constexpr uint8_t kTotalLengthOffset        = 2;
    static constexpr uint8_t kIdentificationOffset     = 4;
    static constexpr uint8_t kFlagsFragmentOffset      = 6;
    static constexpr uint8_t kTtlOffset                = 8;
    static constexpr uint8_t kProtocolOffset           = 9;
    static constexpr uint8_t kHeaderChecksumOffset     = 10;
    static constexpr uint8_t kSourceAddressOffset      = 12;
    static constexpr uint8_t kDestinationAddressOffset = 16;

    /**
     * Indicates whether or not the header appears to be well-formed.
     *
     * @retval TRUE    If the header appears to be well-formed.
     * @retval FALSE   If the header does not appear to be well-formed.
     *
     */
    bool IsValid(void) const { return IsVersion4(); }

    /**
     * Initializes the Version to 4 and sets Traffic Class and Flow fields to zero.
     *
     * The other fields in the IPv4 header remain unchanged.
     *
     */
    void InitVersionIhl(void) { SetVersionIhl(kVersIhlInit); }

    /**
     * Sets the version and Ihl of the IPv4 header.
     *
     * @param[in] aVersionIhl The octet for the version and Ihl field.
     *
     */
    void SetVersionIhl(uint8_t aVersionIhl) { mVersIhl = aVersionIhl; }

    /**
     * Indicates whether or not the IPv4 Version is set to 6.
     *
     * @retval TRUE   If the IPv4 Version is set to 4.
     * @retval FALSE  If the IPv4 Version is not set to 4.
     *
     */
    bool IsVersion4(void) const { return (mVersIhl & kVersionMask) == kVersion4; }

    /**
     * Returns the octet for DSCP + ECN.
     *
     * @retval The octet for DSCP and ECN.
     *
     */
    uint8_t GetDscpEcn(void) const { return mDscpEcn; }

    /**
     * Gets the 6-bit Differentiated Services Code Point (DSCP) from Traffic Class field.
     *
     * @returns The DSCP value.
     *
     */
    uint8_t GetDscp(void) const { return (mDscpEcn & kDscpMask) >> kDscpOffset; }

    /**
     * Sets 6-bit Differentiated Services Code Point (DSCP) in IPv4 header.
     *
     * @param[in]  aDscp  The DSCP value.
     *
     */
    void SetDscp(uint8_t aDscp) { mDscpEcn = static_cast<uint8_t>((mDscpEcn & ~kDscpMask) | (aDscp << kDscpOffset)); }

    /**
     * Gets the 2-bit Explicit Congestion Notification (ECN) from Traffic Class field.
     *
     * @returns The ECN value.
     *
     */
    Ecn GetEcn(void) const { return static_cast<Ecn>(mDscpEcn & kEcnMask); }

    /**
     * Sets the 2-bit Explicit Congestion Notification (ECN) in IPv4 header..
     *
     * @param[in]  aEcn  The ECN value.
     *
     */
    void SetEcn(Ecn aEcn) { mDscpEcn = ((mDscpEcn & ~kEcnMask) | aEcn); }

    /**
     * Returns the IPv4 Payload Length value.
     *
     * @returns The IPv4 Payload Length value.
     *
     */
    uint16_t GetTotalLength(void) const { return BigEndian::HostSwap16(mTotalLength); }

    /**
     * Sets the IPv4 Payload Length value.
     *
     * @param[in]  aLength  The IPv4 Payload Length value.
     *
     */
    void SetTotalLength(uint16_t aLength) { mTotalLength = BigEndian::HostSwap16(aLength); }

    /**
     * Returns the IPv4 payload protocol.
     *
     * @returns The IPv4 payload protocol value.
     *
     */
    uint8_t GetProtocol(void) const { return mProtocol; }

    /**
     * Sets the IPv4 payload protocol.
     *
     * @param[in]  aProtocol  The IPv4 payload protocol.
     *
     */
    void SetProtocol(uint8_t aProtocol) { mProtocol = aProtocol; }

    /**
     * Returns the IPv4 header checksum, the checksum is in host endian.
     *
     * @returns The checksum field in the IPv4 header.
     *
     */
    uint16_t GetChecksum(void) const { return BigEndian::HostSwap16(mHeaderChecksum); }

    /**
     * Sets the IPv4 header checksum, the checksum is in host endian.
     *
     * @param[in] aChecksum The checksum for the IPv4 header.
     *
     */
    void SetChecksum(uint16_t aChecksum) { mHeaderChecksum = BigEndian::HostSwap16(aChecksum); }

    /**
     * Returns the IPv4 Identification value.
     *
     * @returns The IPv4 Identification value.
     *
     */
    uint16_t GetIdentification(void) const { return BigEndian::HostSwap16(mIdentification); }

    /**
     * Sets the IPv4 Identification value.
     *
     * @param[in] aIdentification The IPv4 Identification value.
     *
     */
    void SetIdentification(uint16_t aIdentification) { mIdentification = BigEndian::HostSwap16(aIdentification); }

    /**
     * Returns the IPv4 Time-to-Live value.
     *
     * @returns The IPv4 Time-to-Live value.
     *
     */
    uint8_t GetTtl(void) const { return mTtl; }

    /**
     * Sets the IPv4 Time-to-Live value.
     *
     * @param[in]  aTtl  The IPv4 Time-to-Live value.
     *
     */
    void SetTtl(uint8_t aTtl) { mTtl = aTtl; }

    /**
     * Returns the IPv4 Source address.
     *
     * @returns A reference to the IPv4 Source address.
     *
     */
    Address &GetSource(void) { return mSource; }

    /**
     * Returns the IPv4 Source address.
     *
     * @returns A reference to the IPv4 Source address.
     *
     */
    const Address &GetSource(void) const { return mSource; }

    /**
     * Sets the IPv4 Source address.
     *
     * @param[in]  aSource  A reference to the IPv4 Source address.
     *
     */
    void SetSource(const Address &aSource) { mSource = aSource; }

    /**
     * Returns the IPv4 Destination address.
     *
     * @returns A reference to the IPv4 Destination address.
     *
     */
    Address &GetDestination(void) { return mDestination; }

    /**
     * Returns the IPv4 Destination address.
     *
     * @returns A reference to the IPv4 Destination address.
     *
     */
    const Address &GetDestination(void) const { return mDestination; }

    /**
     * Sets the IPv4 Destination address.
     *
     * @param[in]  aDestination  A reference to the IPv4 Destination address.
     *
     */
    void SetDestination(const Address &aDestination) { mDestination = aDestination; }

    /**
     * Parses and validates the IPv4 header from a given message.
     *
     * The header is read from @p aMessage at offset zero.
     *
     * @param[in]  aMessage  The IPv4 message.
     *
     * @retval kErrorNone   Successfully parsed the IPv4 header from @p aMessage.
     * @retval kErrorParse  Malformed IPv4 header or message (e.g., message does not contained expected payload length).
     *
     */
    Error ParseFrom(const Message &aMessage);

    /**
     * Returns the Df flag in the IPv4 header.
     *
     * @returns Whether don't fragment flag is set.
     *
     */
    bool GetDf(void) const { return BigEndian::HostSwap16(mFlagsFragmentOffset) & kFlagsDf; }

    /**
     * Returns the Mf flag in the IPv4 header.
     *
     * @returns Whether more fragments flag is set.
     *
     */
    bool GetMf(void) const { return BigEndian::HostSwap16(mFlagsFragmentOffset) & kFlagsMf; }

    /**
     * Returns the fragment offset in the IPv4 header.
     *
     * @returns The fragment offset of the IPv4 packet.
     *
     */
    uint16_t GetFragmentOffset(void) const { return BigEndian::HostSwap16(mFlagsFragmentOffset) & kFragmentOffsetMask; }

private:
    // IPv4 header
    //
    // +---------------+---------------+---------------+---------------+
    // |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |Version|  IHL  |    DSCP   |ECN|         Total Length          |
    // |        Identification         |Flags|    Fragment Offset      |
    // |      TTL      |    Protocol   |        Header Checksum        |
    // |                       Source IP Address                       |
    // |                         Dest IP Address                       |
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    static constexpr uint8_t  kVersion4           = 0x40;   // Use with `mVersIhl`
    static constexpr uint8_t  kVersionMask        = 0xf0;   // Use with `mVersIhl`
    static constexpr uint8_t  kIhlMask            = 0x0f;   // Use with `mVersIhl`
    static constexpr uint8_t  kDscpOffset         = 2;      // Use with `mDscpEcn`
    static constexpr uint16_t kDscpMask           = 0xfc;   // Use with `mDscpEcn`
    static constexpr uint8_t  kEcnOffset          = 0;      // Use with `mDscpEcn`
    static constexpr uint8_t  kEcnMask            = 0x03;   // Use with `mDscpEcn`
    static constexpr uint16_t kFlagsMask          = 0xe000; // Use with `mFlagsFragmentOffset`
    static constexpr uint16_t kFlagsDf            = 0x4000; // Use with `mFlagsFragmentOffset`
    static constexpr uint16_t kFlagsMf            = 0x2000; // Use with `mFlagsFragmentOffset`
    static constexpr uint16_t kFragmentOffsetMask = 0x1fff; // Use with `mFlagsFragmentOffset`
    static constexpr uint32_t kVersIhlInit        = 0x45;   // Version 4, Header length = 5x8 bytes.

    uint8_t  mVersIhl;
    uint8_t  mDscpEcn;
    uint16_t mTotalLength;
    uint16_t mIdentification;
    uint16_t mFlagsFragmentOffset;
    uint8_t  mTtl;
    uint8_t  mProtocol;
    uint16_t mHeaderChecksum;
    Address  mSource;
    Address  mDestination;
} OT_TOOL_PACKED_END;

/**
 * Implements ICMP(v4).
 * Note: ICMP(v4) messages will only be generated / handled by NAT64. So only header definition is required.
 *
 */
class Icmp
{
public:
    /**
     * Represents an IPv4 ICMP header.
     *
     */
    OT_TOOL_PACKED_BEGIN
    class Header : public Clearable<Header>
    {
    public:
        static constexpr uint16_t kChecksumFieldOffset = 2;
        // A few ICMP types, only the ICMP types work with NAT64 are listed here.
        enum Type : uint8_t
        {
            kTypeEchoReply              = 0,
            kTypeDestinationUnreachable = 3,
            kTypeEchoRequest            = 8,
            kTypeTimeExceeded           = 11,
        };

        enum Code : uint8_t
        {
            kCodeNone = 0,
            // Destination Unreachable codes
            kCodeNetworkUnreachable  = 0,
            kCodeHostUnreachable     = 1,
            kCodeProtocolUnreachable = 2,
            kCodePortUnreachable     = 3,
            kCodeSourceRouteFailed   = 5,
            kCodeNetworkUnknown      = 6,
            kCodeHostUnknown         = 7,
        };

        /**
         * Returns the type of the ICMP message.
         *
         * @returns The type field of the ICMP message.
         *
         */
        uint8_t GetType(void) const { return mType; }

        /**
         * Sets the type of the ICMP message.
         *
         * @param[in] aType The type of the ICMP message.
         *
         */
        void SetType(uint8_t aType) { mType = aType; }

        /**
         * Returns the code of the ICMP message.
         *
         * @returns The code field of the ICMP message.
         *
         */
        uint8_t GetCode(void) const { return mCode; }

        /**
         * Sets the code of the ICMP message.
         *
         * @param[in] aCode The code of the ICMP message.
         *
         */
        void SetCode(uint8_t aCode) { mCode = aCode; }

        /**
         * Sets the checksum field in the ICMP message.
         *
         * @returns The checksum of the ICMP message.
         *
         */
        uint16_t GetChecksum(void) const { return BigEndian::HostSwap16(mChecksum); }

        /**
         * Sets the checksum field in the ICMP message.
         *
         * @param[in] aChecksum The checksum of the ICMP message.
         *
         */
        void SetChecksum(uint16_t aChecksum) { mChecksum = BigEndian::HostSwap16(aChecksum); }

        /**
         * Returns the rest of header field in the ICMP message.
         *
         * @returns The rest of header field in the ICMP message. The returned buffer has 4 octets.
         *
         */
        const uint8_t *GetRestOfHeader(void) const { return mRestOfHeader; }

        /**
         * Sets the rest of header field in the ICMP message.
         *
         * @param[in] aRestOfHeader The rest of header field in the ICMP message. The buffer should have 4 octets.
         *
         */
        void SetRestOfHeader(const uint8_t *aRestOfHeader)
        {
            memcpy(mRestOfHeader, aRestOfHeader, sizeof(mRestOfHeader));
        }

    private:
        uint8_t  mType;
        uint8_t  mCode;
        uint16_t mChecksum;
        uint8_t  mRestOfHeader[4];
    } OT_TOOL_PACKED_END;
};

// Internet Protocol Numbers
static constexpr uint8_t kProtoTcp  = Ip6::kProtoTcp; ///< Transmission Control Protocol
static constexpr uint8_t kProtoUdp  = Ip6::kProtoUdp; ///< User Datagram
static constexpr uint8_t kProtoIcmp = 1;              ///< ICMP for IPv4

using Tcp = Ip6::Tcp; // TCP in IPv4 is the same as TCP in IPv6
using Udp = Ip6::Udp; // UDP in IPv4 is the same as UDP in IPv6

/**
 * @}
 *
 */

} // namespace Ip4

DefineCoreType(otIp4Address, Ip4::Address);
DefineCoreType(otIp4Cidr, Ip4::Cidr);

} // namespace ot

#endif // IP4_TYPES_HPP_
