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
 *   This file includes definitions for IPv6 packet processing.
 */

#ifndef IP6_HEADERS_HPP_
#define IP6_HEADERS_HPP_

#include "openthread-core-config.h"

#include <stddef.h>

#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/message.hpp"
#include "net/ip6_address.hpp"
#include "net/ip6_types.hpp"
#include "net/netif.hpp"
#include "net/socket.hpp"

namespace ot {

/**
 * @namespace ot::Ip6
 *
 * @brief
 *   This namespace includes definitions for IPv6 networking.
 */
namespace Ip6 {

/**
 * @addtogroup core-ipv6
 *
 * @brief
 *   This module includes definitions for the IPv6 network layer.
 *
 * @{
 *
 * @defgroup core-ip6-icmp6 ICMPv6
 * @defgroup core-ip6-ip6 IPv6
 * @defgroup core-ip6-mpl MPL
 * @defgroup core-ip6-netif Network Interfaces
 *
 * @}
 */

/**
 * @addtogroup core-ip6-ip6
 *
 * @brief
 *   This module includes definitions for core IPv6 networking.
 *
 * @{
 */

/**
 * Implements IPv6 header generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class Header : public Clearable<Header>
{
public:
    static constexpr uint8_t kPayloadLengthFieldOffset = 4;  ///< Offset of Payload Length field in IPv6 header.
    static constexpr uint8_t kNextHeaderFieldOffset    = 6;  ///< Offset of Next Header field in IPv6 header.
    static constexpr uint8_t kHopLimitFieldOffset      = 7;  ///< Offset of Hop Limit field in IPv6 header.
    static constexpr uint8_t kSourceFieldOffset        = 8;  ///< Offset of Source Address field in IPv6 header.
    static constexpr uint8_t kDestinationFieldOffset   = 24; ///< Offset of Destination Address field in IPv6 header.

    /**
     * Initializes the Version to 6 and sets Traffic Class and Flow fields to zero.
     *
     * The other fields in the IPv6 header remain unchanged.
     */
    void InitVersionTrafficClassFlow(void) { SetVerionTrafficClassFlow(kVersTcFlowInit); }

    /**
     * Indicates whether or not the header appears to be well-formed.
     *
     * @retval TRUE    If the header appears to be well-formed.
     * @retval FALSE   If the header does not appear to be well-formed.
     */
    bool IsValid(void) const;

    /**
     * Indicates whether or not the IPv6 Version is set to 6.
     *
     * @retval TRUE   If the IPv6 Version is set to 6.
     * @retval FALSE  If the IPv6 Version is not set to 6.
     */
    bool IsVersion6(void) const { return (mVerTcFlow.m8[0] & kVersionMask) == kVersion6; }

    /**
     * Gets the combination of Version, Traffic Class, and Flow fields as a 32-bit value.
     *
     * @returns The Version, Traffic Class, and Flow fields as a 32-bit value.
     */
    uint32_t GetVerionTrafficClassFlow(void) const { return BigEndian::HostSwap32(mVerTcFlow.m32); }

    /**
     * Sets the combination of Version, Traffic Class, and Flow fields as a 32-bit value.
     *
     * @param[in] aVerTcFlow   The Version, Traffic Class, and Flow fields as a 32-bit value.
     */
    void SetVerionTrafficClassFlow(uint32_t aVerTcFlow) { mVerTcFlow.m32 = BigEndian::HostSwap32(aVerTcFlow); }

    /**
     * Gets the Traffic Class field.
     *
     * @returns The Traffic Class field.
     */
    uint8_t GetTrafficClass(void) const
    {
        return static_cast<uint8_t>((BigEndian::HostSwap16(mVerTcFlow.m16[0]) & kTrafficClassMask) >>
                                    kTrafficClassOffset);
    }

    /**
     * Sets the Traffic Class filed.
     *
     * @param[in] aTc  The Traffic Class value.
     */
    void SetTrafficClass(uint8_t aTc)
    {
        mVerTcFlow.m16[0] =
            BigEndian::HostSwap16((BigEndian::HostSwap16(mVerTcFlow.m16[0]) & ~kTrafficClassMask) |
                                  ((static_cast<uint16_t>(aTc) << kTrafficClassOffset) & kTrafficClassMask));
    }

    /**
     * Gets the 6-bit Differentiated Services Code Point (DSCP) from Traffic Class field.
     *
     * @returns The DSCP value.
     */
    uint8_t GetDscp(void) const
    {
        return static_cast<uint8_t>((BigEndian::HostSwap16(mVerTcFlow.m16[0]) & kDscpMask) >> kDscpOffset);
    }

    /**
     * Sets 6-bit Differentiated Services Code Point (DSCP) in IPv6 header.
     *
     * @param[in]  aDscp  The DSCP value.
     */
    void SetDscp(uint8_t aDscp)
    {
        mVerTcFlow.m16[0] = BigEndian::HostSwap16((BigEndian::HostSwap16(mVerTcFlow.m16[0]) & ~kDscpMask) |
                                                  ((static_cast<uint16_t>(aDscp) << kDscpOffset) & kDscpMask));
    }

    /**
     * Gets the 2-bit Explicit Congestion Notification (ECN) from Traffic Class field.
     *
     * @returns The ECN value.
     */
    Ecn GetEcn(void) const { return static_cast<Ecn>((mVerTcFlow.m8[1] & kEcnMask) >> kEcnOffset); }

    /**
     * Sets the 2-bit Explicit Congestion Notification (ECN) in IPv6 header..
     *
     * @param[in]  aEcn  The ECN value.
     */
    void SetEcn(Ecn aEcn) { mVerTcFlow.m8[1] = (mVerTcFlow.m8[1] & ~kEcnMask) | ((aEcn << kEcnOffset) & kEcnMask); }

    /**
     * Gets the 20-bit Flow field.
     *
     * @returns  The Flow value.
     */
    uint32_t GetFlow(void) const { return BigEndian::HostSwap32(mVerTcFlow.m32) & kFlowMask; }

    /**
     * Sets the 20-bit Flow field in IPv6 header.
     *
     * @param[in] aFlow  The Flow value.
     */
    void SetFlow(uint32_t aFlow)
    {
        mVerTcFlow.m32 =
            BigEndian::HostSwap32((BigEndian::HostSwap32(mVerTcFlow.m32) & ~kFlowMask) | (aFlow & kFlowMask));
    }

    /**
     * Returns the IPv6 Payload Length value.
     *
     * @returns The IPv6 Payload Length value.
     */
    uint16_t GetPayloadLength(void) const { return BigEndian::HostSwap16(mPayloadLength); }

    /**
     * Sets the IPv6 Payload Length value.
     *
     * @param[in]  aLength  The IPv6 Payload Length value.
     */
    void SetPayloadLength(uint16_t aLength) { mPayloadLength = BigEndian::HostSwap16(aLength); }

    /**
     * Returns the IPv6 Next Header value.
     *
     * @returns The IPv6 Next Header value.
     */
    uint8_t GetNextHeader(void) const { return mNextHeader; }

    /**
     * Sets the IPv6 Next Header value.
     *
     * @param[in]  aNextHeader  The IPv6 Next Header value.
     */
    void SetNextHeader(uint8_t aNextHeader) { mNextHeader = aNextHeader; }

    /**
     * Returns the IPv6 Hop Limit value.
     *
     * @returns The IPv6 Hop Limit value.
     */
    uint8_t GetHopLimit(void) const { return mHopLimit; }

    /**
     * Sets the IPv6 Hop Limit value.
     *
     * @param[in]  aHopLimit  The IPv6 Hop Limit value.
     */
    void SetHopLimit(uint8_t aHopLimit) { mHopLimit = aHopLimit; }

    /**
     * Returns the IPv6 Source address.
     *
     * @returns A reference to the IPv6 Source address.
     */
    Address &GetSource(void) { return mSource; }

    /**
     * Returns the IPv6 Source address.
     *
     * @returns A reference to the IPv6 Source address.
     */
    const Address &GetSource(void) const { return mSource; }

    /**
     * Sets the IPv6 Source address.
     *
     * @param[in]  aSource  A reference to the IPv6 Source address.
     */
    void SetSource(const Address &aSource) { mSource = aSource; }

    /**
     * Returns the IPv6 Destination address.
     *
     * @returns A reference to the IPv6 Destination address.
     */
    Address &GetDestination(void) { return mDestination; }

    /**
     * Returns the IPv6 Destination address.
     *
     * @returns A reference to the IPv6 Destination address.
     */
    const Address &GetDestination(void) const { return mDestination; }

    /**
     * Sets the IPv6 Destination address.
     *
     * @param[in]  aDestination  A reference to the IPv6 Destination address.
     */
    void SetDestination(const Address &aDestination) { mDestination = aDestination; }

    /**
     * Parses and validates the IPv6 header from a given message.
     *
     * The header is read from @p aMessage at offset zero.
     *
     * @param[in]  aMessage  The IPv6 message.
     *
     * @retval kErrorNone   Successfully parsed the IPv6 header from @p aMessage.
     * @retval kErrorParse  Malformed IPv6 header or message (e.g., message does not contained expected payload length).
     */
    Error ParseFrom(const Message &aMessage);

private:
    // IPv6 header `mVerTcFlow` field:
    //
    // |             m16[0]            |            m16[1]             |
    // |     m8[0]     |     m8[1]     |     m8[2]     |      m8[3]    |
    // +---------------+---------------+---------------+---------------+
    // |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |Version|    DSCP   |ECN|             Flow Label                |
    // |       | Traffic Class |                                       |
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    static constexpr uint8_t  kVersion6           = 0x60;       // Use with `mVerTcFlow.m8[0]`
    static constexpr uint8_t  kVersionMask        = 0xf0;       // Use with `mVerTcFlow.m8[0]`
    static constexpr uint8_t  kTrafficClassOffset = 4;          // Use with `mVerTcFlow.m16[0]`
    static constexpr uint16_t kTrafficClassMask   = 0x0ff0;     // Use with `mVerTcFlow.m16[0]`
    static constexpr uint8_t  kDscpOffset         = 6;          // Use with `mVerTcFlow.m16[0]`
    static constexpr uint16_t kDscpMask           = 0x0fc0;     // Use with `mVerTcFlow.m16[0]`
    static constexpr uint8_t  kEcnOffset          = 4;          // Use with `mVerTcFlow.m8[1]`
    static constexpr uint8_t  kEcnMask            = 0x30;       // Use with `mVerTcFlow.m8[1]`
    static constexpr uint32_t kFlowMask           = 0x000fffff; // Use with `mVerTcFlow.m32`
    static constexpr uint32_t kVersTcFlowInit     = 0x60000000; // Version 6, TC and flow zero.

    union OT_TOOL_PACKED_FIELD
    {
        uint8_t  m8[sizeof(uint32_t) / sizeof(uint8_t)];
        uint16_t m16[sizeof(uint32_t) / sizeof(uint16_t)];
        uint32_t m32;
    } mVerTcFlow;
    uint16_t mPayloadLength;
    uint8_t  mNextHeader;
    uint8_t  mHopLimit;
    Address  mSource;
    Address  mDestination;
} OT_TOOL_PACKED_END;

/**
 * Implements IPv6 Extension Header generation and processing.
 */
OT_TOOL_PACKED_BEGIN
class ExtensionHeader
{
public:
    /**
     * This constant defines the size of Length unit in bytes.
     *
     * The Length field is in 8-bytes unit. The total size of `ExtensionHeader` MUST be a multiple of 8.
     */
    static constexpr uint16_t kLengthUnitSize = 8;

    /**
     * Returns the IPv6 Next Header value.
     *
     * @returns The IPv6 Next Header value.
     */
    uint8_t GetNextHeader(void) const { return mNextHeader; }

    /**
     * Sets the IPv6 Next Header value.
     *
     * @param[in]  aNextHeader  The IPv6 Next Header value.
     */
    void SetNextHeader(uint8_t aNextHeader) { mNextHeader = aNextHeader; }

    /**
     * Returns the IPv6 Header Extension Length value.
     *
     * The Length is in 8-byte units and does not include the first 8 bytes.
     *
     * @returns The IPv6 Header Extension Length value.
     */
    uint8_t GetLength(void) const { return mLength; }

    /**
     * Sets the IPv6 Header Extension Length value.
     *
     * The Length is in 8-byte units and does not include the first 8 bytes.
     *
     * @param[in]  aLength  The IPv6 Header Extension Length value.
     */
    void SetLength(uint8_t aLength) { mLength = aLength; }

    /**
     * Returns the size (number of bytes) of the Extension Header including Next Header and Length fields.
     *
     * @returns The size (number of bytes) of the Extension Header.
     */
    uint16_t GetSize(void) const { return kLengthUnitSize * (mLength + 1); }

private:
    // |     m8[0]     |     m8[1]     |     m8[2]     |      m8[3]    |
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // | Next Header   | Header Length | . . .                         |
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    uint8_t mNextHeader;
    uint8_t mLength;
} OT_TOOL_PACKED_END;

/**
 * Implements IPv6 Hop-by-Hop Options Header generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class HopByHopHeader : public ExtensionHeader
{
} OT_TOOL_PACKED_END;

/**
 * Implements IPv6 Options generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class Option
{
public:
    /**
     * IPv6 Option Type actions for unrecognized IPv6 Options.
     */
    enum Action : uint8_t
    {
        kActionSkip      = 0x00, ///< Skip over this option and continue processing the header.
        kActionDiscard   = 0x40, ///< Discard the packet.
        kActionForceIcmp = 0x80, ///< Discard the packet and forcibly send an ICMP Parameter Problem.
        kActionIcmp      = 0xc0, ///< Discard packet and conditionally send an ICMP Parameter Problem.
    };

    /**
     * Returns the IPv6 Option Type value.
     *
     * @returns The IPv6 Option Type value.
     */
    uint8_t GetType(void) const { return mType; }

    /**
     * Indicates whether IPv6 Option is padding (either Pad1 or PadN).
     *
     * @retval TRUE   The Option is padding.
     * @retval FALSE  The Option is not padding.
     */
    bool IsPadding(void) const { return (mType == kTypePad1) || (mType == kTypePadN); }

    /**
     * Returns the IPv6 Option action for unrecognized IPv6 Options.
     *
     * @returns The IPv6 Option action for unrecognized IPv6 Options.
     */
    Action GetAction(void) const { return static_cast<Action>(mType & kActionMask); }

    /**
     * Returns the IPv6 Option Length value.
     *
     * @returns The IPv6 Option Length value.
     */
    uint8_t GetLength(void) const { return mLength; }

    /**
     * Returns the size (number of bytes) of the IPv6 Option.
     *
     * Returns the proper size of the Option independent of its type, particularly if Option is Pad1 (which
     * does not follow the common Option header structure and has only Type field with no Length field). For other
     * Option types, the returned size includes the Type and Length fields.
     *
     * @returns The size of the Option.
     */
    uint16_t GetSize(void) const;

    /**
     * Parses and validates the IPv6 Option from a given message.
     *
     * The Option is read from @p aOffset in @p aMessage. This method then checks that the entire Option is present
     * within @p aOffsetRange.
     *
     * @param[in]  aMessage      The IPv6 message.
     * @param[in]  aOffsetRange  The offset range in @p aMessage to read the IPv6 Option.
     *
     * @retval kErrorNone   Successfully parsed the IPv6 option from @p aMessage.
     * @retval kErrorParse  Malformed IPv6 Option or Option is not contained within @p aMessage and @p aOffsetRange.
     */
    Error ParseFrom(const Message &aMessage, const OffsetRange &aOffsetRange);

protected:
    static constexpr uint8_t kTypePad1 = 0x00; ///< Pad1 Option Type.
    static constexpr uint8_t kTypePadN = 0x01; ///< PanN Option Type.

    /**
     * Sets the IPv6 Option Type value.
     *
     * @param[in]  aType  The IPv6 Option Type value.
     */
    void SetType(uint8_t aType) { mType = aType; }

    /**
     * Sets the IPv6 Option Length value.
     *
     * @param[in]  aLength  The IPv6 Option Length value.
     */
    void SetLength(uint8_t aLength) { mLength = aLength; }

private:
    static constexpr uint8_t kActionMask = 0xc0;

    uint8_t mType;
    uint8_t mLength;
} OT_TOOL_PACKED_END;

/**
 * Implements IPv6 Pad Options (Pad1 or PadN) generation.
 */
OT_TOOL_PACKED_BEGIN
class PadOption : public Option, private Clearable<PadOption>
{
    friend class Clearable<PadOption>;

public:
    /**
     * Initializes the Pad Option for a given total Pad size.
     *
     * The @p aPadSize MUST be from range 1-7. Otherwise the behavior of this method is undefined.
     *
     * @param[in]  aPadSize  The total number of needed padding bytes.
     */
    void InitForPadSize(uint8_t aPadSize);

    /**
     * Initializes the Pad Option for padding an IPv6 Extension header with a given current size.
     *
     * The Extension Header Length is in 8-bytes unit, so the total size should be a multiple of 8. This method
     * determines the Pad Option size needed for appending to Extension Header based on it current size @p aHeaderSize
     * so to make it a multiple of 8. This method returns `kErrorAlready` when the @p aHeaderSize is already
     * a multiple of 8 (i.e., no padding is needed).
     *
     * @param[in] aHeaderSize  The current IPv6 Extension header size (in bytes).
     *
     * @retval kErrorNone     The Pad Option is successfully initialized.
     * @retval kErrorAlready  The @p aHeaderSize is already a multiple of 8 and no padding is needed.
     */
    Error InitToPadHeaderWithSize(uint16_t aHeaderSize);

private:
    static constexpr uint8_t kMaxLength = 5;

    uint8_t mPads[kMaxLength];
} OT_TOOL_PACKED_END;

/**
 * Implements IPv6 Fragment Header generation and parsing.
 */
OT_TOOL_PACKED_BEGIN
class FragmentHeader
{
public:
    /**
     * Initializes the IPv6 Fragment header.
     */
    void Init(void)
    {
        mReserved       = 0;
        mOffsetMore     = 0;
        mIdentification = 0;
    }

    /**
     * Returns the IPv6 Next Header value.
     *
     * @returns The IPv6 Next Header value.
     */
    uint8_t GetNextHeader(void) const { return mNextHeader; }

    /**
     * Sets the IPv6 Next Header value.
     *
     * @param[in]  aNextHeader  The IPv6 Next Header value.
     */
    void SetNextHeader(uint8_t aNextHeader) { mNextHeader = aNextHeader; }

    /**
     * Returns the Fragment Offset value.
     *
     * @returns The Fragment Offset value.
     */
    uint16_t GetOffset(void) const { return (BigEndian::HostSwap16(mOffsetMore) & kOffsetMask) >> kOffsetOffset; }

    /**
     * Sets the Fragment Offset value.
     *
     * @param[in]  aOffset  The Fragment Offset value.
     */
    void SetOffset(uint16_t aOffset)
    {
        uint16_t tmp = BigEndian::HostSwap16(mOffsetMore);
        tmp          = (tmp & ~kOffsetMask) | ((aOffset << kOffsetOffset) & kOffsetMask);
        mOffsetMore  = BigEndian::HostSwap16(tmp);
    }

    /**
     * Returns the M flag value.
     *
     * @returns The M flag value.
     */
    bool IsMoreFlagSet(void) const { return BigEndian::HostSwap16(mOffsetMore) & kMoreFlag; }

    /**
     * Clears the M flag value.
     */
    void ClearMoreFlag(void) { mOffsetMore = BigEndian::HostSwap16(BigEndian::HostSwap16(mOffsetMore) & ~kMoreFlag); }

    /**
     * Sets the M flag value.
     */
    void SetMoreFlag(void) { mOffsetMore = BigEndian::HostSwap16(BigEndian::HostSwap16(mOffsetMore) | kMoreFlag); }

    /**
     * Returns the frame identification.
     *
     * @returns The frame identification.
     */
    uint32_t GetIdentification(void) const { return mIdentification; }

    /**
     * Sets the frame identification.
     *
     * @param[in]  aIdentification  The fragment identification value.
     */
    void SetIdentification(uint32_t aIdentification) { mIdentification = aIdentification; }

    /**
     * Returns the next valid payload length for a fragment.
     *
     * @param[in]  aLength  The payload length to be validated for a fragment.
     *
     * @returns Valid IPv6 fragment payload length.
     */
    static inline uint16_t MakeDivisibleByEight(uint16_t aLength) { return aLength & 0xfff8; }

    /**
     * Converts the fragment offset of 8-octet units into bytes.
     *
     * @param[in]  aOffset  The fragment offset in 8-octet units.
     *
     * @returns The fragment offset in bytes.
     */
    static inline uint16_t FragmentOffsetToBytes(uint16_t aOffset) { return static_cast<uint16_t>(aOffset << 3); }

    /**
     * Converts a fragment offset in bytes into a fragment offset in 8-octet units.
     *
     * @param[in]  aOffset  The fragment offset in bytes.
     *
     * @returns The fragment offset in 8-octet units.
     */
    static inline uint16_t BytesToFragmentOffset(uint16_t aOffset) { return aOffset >> 3; }

private:
    static constexpr uint8_t  kOffsetOffset = 3;
    static constexpr uint16_t kOffsetMask   = 0xfff8;
    static constexpr uint16_t kMoreFlag     = 1;

    uint8_t  mNextHeader;
    uint8_t  mReserved;
    uint16_t mOffsetMore;
    uint32_t mIdentification;
} OT_TOOL_PACKED_END;

/**
 * @}
 */

} // namespace Ip6
} // namespace ot

#endif // IP6_HEADERS_HPP_
